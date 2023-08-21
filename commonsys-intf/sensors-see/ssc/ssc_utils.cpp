/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include <string>
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "sns_client.pb.h"
#include "ssc_connection.h"
#include "sensors_log.h"
#include "ssc_utils.h"
#include <cinttypes>
#include <cmath>
#include <chrono>
#include <cutils/properties.h>
#include <fstream>
#include <string>
#include "sns_client.pb.h"
#include "sns_suid.pb.h"
using namespace std;
using namespace google::protobuf::io;

using namespace std::chrono;

static const char *SLPI_SSR_SYSFS_PATH
            = "/sys/kernel/boot_slpi/ssr";

static const char *ADSP_SSR_SYSFS_PATH
            = "/sys/kernel/boot_adsp/ssr";

static const char * SNS_SETTINGS_FILE
            = "/mnt/vendor/persist/sensors/sensors_settings";

static const char SENSORS_HAL_PROP_STATS[] ="persist.vendor.sensors.debug.stats";
static const char SENSORS_HAL_PROP_SSC_LATENCY[] ="persist.vendor.sensors.debug.ssc_latency";
static const char SENSORS_HAL_PROP_SSC_TRACE[] ="persist.vendor.sensors.debug.trace";
static const char SENSORS_HAL_PROP_QMI_DEBUG[] ="persist.vendor.sensors.debug.ssc_qmi_debug";

bool sns_trace_enabled = false;

suid_lookup::suid_lookup(suid_event_function cb):
    _cb(cb),
    _ssc_conn(get_ssc_event_cb())
{
    _ssc_conn.set_worker_name("see_suid_lookup");
    _set_thread_name = true;
}

void suid_lookup::request_suid(std::string datatype, bool default_only)
{
    sns_client_request_msg pb_req_msg;
    sns_suid_req pb_suid_req;
    string pb_suid_req_encoded;
    // TODO: add this as an optional parameter to the function

    const sensor_uid LOOKUP_SUID = {
        12370169555311111083ull,
        12370169555311111083ull
    };

    sns_logv("requesting suid for %s, ts = %fs", datatype.c_str(),
             duration_cast<duration<float>>(high_resolution_clock::now().
                                            time_since_epoch()).count());

    /* populate SUID request */
    pb_suid_req.set_data_type(datatype);
    pb_suid_req.set_register_updates(true);
    pb_suid_req.set_default_only(default_only);
    pb_suid_req.SerializeToString(&pb_suid_req_encoded);

    /* populate the client request message */
    pb_req_msg.set_msg_id(SNS_SUID_MSGID_SNS_SUID_REQ);
    pb_req_msg.mutable_request()->set_payload(pb_suid_req_encoded);
    pb_req_msg.mutable_suid()->set_suid_high(LOOKUP_SUID.high);
    pb_req_msg.mutable_suid()->set_suid_low(LOOKUP_SUID.low);
    pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);
    string pb_req_msg_encoded;
    pb_req_msg.SerializeToString(&pb_req_msg_encoded);
    _ssc_conn.send_request(pb_req_msg_encoded);
}

void suid_lookup::handle_ssc_event(const uint8_t *data, size_t size)
{
    if (_set_thread_name == true) {
        _set_thread_name = false;
        int ret_code = 0;
        string pthread_name = "suid_lookup_see";
        sns_logi("thread_name= %s", pthread_name.c_str());
        ret_code = pthread_setname_np(pthread_self(), pthread_name.c_str());
        if (ret_code != 0) {
            sns_loge("Failed to set ThreadName: %s\n", pthread_name.c_str());
        }
    }
    /* parse the pb encoded event */
    sns_client_event_msg pb_event_msg;
    pb_event_msg.ParseFromArray(data, size);
    /* iterate over all events in the message */
    for (int i = 0; i < pb_event_msg.events_size(); i++) {
        auto& pb_event = pb_event_msg.events(i);
        if (pb_event.msg_id() != SNS_SUID_MSGID_SNS_SUID_EVENT) {
            sns_loge("invalid event msg_id=%d", pb_event.msg_id());
            continue;
        }
        sns_suid_event pb_suid_event;
        pb_suid_event.ParseFromString(pb_event.payload());
        const string& datatype =  pb_suid_event.data_type();

        sns_logv("suid_event for %s, num_suids=%d, ts=%fs", datatype.c_str(),
                 pb_suid_event.suid_size(),
                 duration_cast<duration<float>>(high_resolution_clock::now().
                                                time_since_epoch()).count());

        /* create a list of  all suids found for this datatype */
        vector<sensor_uid> suids(pb_suid_event.suid_size());
        for (int j=0; j < pb_suid_event.suid_size(); j++) {
            suids[j] = sensor_uid(pb_suid_event.suid(j).suid_low(),
                                  pb_suid_event.suid(j).suid_high());
        }
        /* send callback for this datatype */
        _cb(datatype, suids);
    }
}

const char SENSORS_HAL_SSR_SUPPORT[] = "persist.vendor.sensors.hal_trigger_ssr";
bool support_ssr_trigger()
{
    char ssr_prop[PROPERTY_VALUE_MAX];
    property_get(SENSORS_HAL_SSR_SUPPORT, ssr_prop, "false");
    sns_logd("ssr_prop: %s",ssr_prop);
    if (!strncmp("true", ssr_prop, 4)) {
        sns_logi("support_ssr_trigger: %s",ssr_prop);
        return true;
    }
    return false;
}

/* utility function to trigger ssr*/
/*  all deamons/system_app do not have permissions to open
     sys/kernel/boot_slpi/ssr , right now only hal_sensors can do it*/
int trigger_ssr() {
    if(support_ssr_trigger() == true) {
        struct stat sb;
        int _fd = -1;
        if (!stat(SLPI_SSR_SYSFS_PATH, &sb))
           _fd = open(SLPI_SSR_SYSFS_PATH, O_WRONLY);
        else
           _fd = open(ADSP_SSR_SYSFS_PATH, O_WRONLY);

        if (_fd<0) {
            sns_logd("failed to open sys/kernel/boot_*/ssr");
            return -1;
        }
        sns_logi("before triggering ssr");
        /*allow some time before calling ssr*/
        sleep(2);
        if (write(_fd, "1", 1) > 0) {
           sns_logi("ssr triggered successfully");
           close(_fd);
           /*allow atleast some time before connecting after ssr*/
           sleep(2);
           return 0;
        } else {
            sns_loge("failed to write sys/kernel/boot_slpi/ssr");
            close(_fd);
            perror("Error: ");
                return -1;
        }
    } else {
        sns_logi("trigger_ssr not supported");
        return -1;
    }
}

/*utility to read the settings related to wakeup in
/perist/registry/registry/sensor_settings*/
uint32_t get_sns_settings() {
    uint32_t settings = 0x0;
    std::ifstream file(SNS_SETTINGS_FILE);
    std::string str;
    /*right now first line but make it generic to add or remove any algo*/
    while (std::getline(file, str))
    {
        sns_logd("sensors_settings line: %s", str.c_str());
        if ( !(settings & DISABLE_SENSORS_FLAG) &&
                    !(str.compare(DISABLE_SENSORS_STRING)))
            settings |= DISABLE_SENSORS_FLAG;

        if ( !(settings & DISABLE_WAKEUP_SENSORS_FLAG) &&
                    !(str.compare(DISABLE_WAKEUP_SENSORS_STRING)))
            settings |= DISABLE_WAKEUP_SENSORS_FLAG;

        if ( !(settings & DISABLE_PROXIMITY_SENSORS_FLAG) &&
                    !(str.compare(DISABLE_PROXIMITY_SENSORS_STRING)))
            settings |= DISABLE_PROXIMITY_SENSORS_FLAG;

        if ( !(settings & SNS_DIAG_CIRC_BUFF_MODE_FLAG) &&
                    !(str.compare(SNS_DAIG_CIRC_BUFF_MODE_STRING)))
            settings |= SNS_DIAG_CIRC_BUFF_MODE_FLAG;

    }
    sns_logi("settings %d", settings);
    return settings;
}

bool get_stats_support_flag(){
  char stats_debug[PROPERTY_VALUE_MAX];
  property_get(SENSORS_HAL_PROP_STATS, stats_debug, "false");
  sns_logd("latency_debug: %s",stats_debug);
  if (!strncmp("true", stats_debug,4)) {
      sns_logi("support_latency_debug : %s",stats_debug);
      return true;
  }
  return false;
}

bool get_ssc_latency_support_flag(){
  char ssc_latency_debug[PROPERTY_VALUE_MAX];
  property_get(SENSORS_HAL_PROP_SSC_LATENCY, ssc_latency_debug, "false");
  sns_logd("latency_debug: %s",ssc_latency_debug);
  if (!strncmp("true", ssc_latency_debug,4)) {
      sns_logi("support_latency_debug : %s",ssc_latency_debug);
      return true;
  }
  return false;
}

bool check_ssc_trace_support_flag(){
  char ssc_trace_debug[PROPERTY_VALUE_MAX];
  property_get(SENSORS_HAL_PROP_SSC_TRACE, ssc_trace_debug, "false");
  sns_logd("trace_debug: %s",ssc_trace_debug);
  if (!strncmp("true", ssc_trace_debug,4)) {
      sns_logi("support_trace_debug : %s",ssc_trace_debug);
      sns_trace_enabled = true;
      return true;
  }
  return false;
}

bool get_qmi_debug_flag(){
  char qmi_debug[PROPERTY_VALUE_MAX];
  property_get(SENSORS_HAL_PROP_QMI_DEBUG, qmi_debug, "false");
  sns_logd("qmi_debug: %s",qmi_debug);
  if (!strncmp("false", qmi_debug,5)) {
      sns_logi("support_qmi_debug : %s",qmi_debug);
      return false;
  }
  return true;
}
