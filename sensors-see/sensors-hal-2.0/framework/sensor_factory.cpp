/*
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include "sensors_log.h"
#include "sensor_factory.h"
#include "sensors_hal_common.h"
#include <cutils/properties.h>
#include <time.h>
#include <sys/stat.h>

using namespace std;

extern int handle_new_registry_update(void);

static const auto MSEC_PER_SEC = 1000;

/* timeout duration for discovery of ssc sensors for first bootup */
static const auto SENSOR_DISCOVERY_TIMEOUT_FIRST = 7;
/* timeout duration for discovery of ssc sensors for all bootups */
static const auto SENSOR_DISCOVERY_TIMEOUT_REST = 1;
/* additional wait time for discovery of critical sensors */
static const int MANDATORY_SENSOR_WAIT_TIME_SEC = 8;
/* additional wait time for discovery of critical sensors */
static const int NON_DEFAULT_SENSOR_WAIT_TIME_MSEC = 500;
/*wait for registry sensor up*/
static int REGISTRY_UP_WAITTIME_SEC = 1;
static int REGISTRY_RETRY_CNT = 80;

static vector<string> mandatory_sensor_datatypes;

static std::mutex mandatory_sensors_mutex;
static map<string, bool> mandatory_sensors_map;
static size_t num_mandatory_sensors_found = 0;

static atomic<bool> all_mandatory_sensors_found { false };

/* timeout duration for receiving attributes for a sensor */
static const uint64_t SENSOR_GET_ATTR_TIMEOUT_NS = 3000000000;
static const int GET_ATTR_NUM_ITERS = 100;

#define HAL_PROP_SIMULATE_SSCTRIGGER "vendor.slpi.ssrsimulate"

static const char* mandatory_sensors_list_file = "/mnt/vendor/persist/sensors/sensors_list.txt";

/* version is part of the sns_reg_config  if updated clears the metadata file*/
static const char* present_registry_version = "/mnt/vendor/persist/sensors/registry/sns_reg_version";
static const char* registry_metadata_file = "/mnt/vendor/persist/sensors/registry/registry/sns_reg_config";
static const char* registry_config_file = "/vendor/etc/sensors/sns_reg_config";

/*following properties are accessble from HAL service only right now*/
bool ssr_simulate() {
    char ssr_trigger[PROPERTY_VALUE_MAX] = "false";
    property_get( HAL_PROP_SIMULATE_SSCTRIGGER, ssr_trigger, "false" );
    if (!strncmp(ssr_trigger, "true", 4)) {
        /* reset to false so that simulation happens only once */
        property_set( HAL_PROP_SIMULATE_SSCTRIGGER, "false");
        return true;
    }
    return false;
}

uint32_t sensor_factory::get_discovery_timeout_ms()
{
    if (_laterboot) {
        return SENSOR_DISCOVERY_TIMEOUT_REST * MSEC_PER_SEC;
    } else {
        return SENSOR_DISCOVERY_TIMEOUT_FIRST * MSEC_PER_SEC;
    }
}

void sensor_factory::init_mandatory_list_db()
{
    struct stat buf;
    _laterboot = (stat(mandatory_sensors_list_file, &buf) == 0);

   if (!_laterboot) {
       sns_logi("first boot after flash");
   }
   if(_laterboot) {
       _read.open(mandatory_sensors_list_file, std::ofstream::in);
        if (_read.fail()) {
           sns_loge("open read fail for sensors_list reading with errno %s ",
           strerror(errno));
           return;
       }

       std::string str;
       while (std::getline(_read, str)){
           auto it = std::find(mandatory_sensor_datatypes.begin() , mandatory_sensor_datatypes.end() , str);
           if(it == mandatory_sensor_datatypes.end()) {
               mandatory_sensor_datatypes.push_back(str);
           } else {
               sns_logi("duplicate entry for %s", str.c_str());
           }
       }
       _write.open(mandatory_sensors_list_file, std::ofstream::app);
        if (_write.fail()) {
           sns_loge("open append fail for sensors_list with errno %s ",
            strerror(errno));
           return;
        }
    } else {
        _write.open(mandatory_sensors_list_file, std::ofstream::out);
         if (_write.fail()) {
            sns_loge("create fail for sensors_list with errno %s ",
            strerror(errno));
            return;
        }
    }
}

void sensor_factory::deinit_mandatory_list_db()
{
    if(_laterboot){
        _write.close();
        _read.close();
    }
    else{
        _write.close();
    }
}

void sensor_factory::update_mandatory_list_database(std::string datatype)
{
    if(_laterboot){
        if(!(std::find(mandatory_sensor_datatypes.begin() , mandatory_sensor_datatypes.end() , datatype) !=
         mandatory_sensor_datatypes.end())){
           mandatory_sensor_datatypes.push_back(datatype);
           _write << datatype + "\n";
           sns_logi("laterboot datatype %s write", datatype.c_str());
        }
    } else {
        if(!(std::find(mandatory_sensor_datatypes.begin() , mandatory_sensor_datatypes.end() , datatype) !=
         mandatory_sensor_datatypes.end())){
            _write << datatype + "\n";
            num_mandatory_sensors_found++;
            mandatory_sensor_datatypes.push_back(datatype);
        }
    }
}

#ifdef NO_FIRMWARE_SUPPORT_FOR_PROPERTY_READING
bool sensor_factory::check_registry_version_change(
                            const char* config_file,
                            int &ret_version)
{
    bool version_changed = false;
    sns_logi("config file is :%s", config_file);
    std::ifstream config_file_version(config_file);
    std::string configfile_version_str;
    std::getline(config_file_version, configfile_version_str);

    if (!configfile_version_str.compare(0, strlen("version="), "version=")) {
        struct stat stat_buf;
        const char *version = configfile_version_str.data()+strlen("version=");

        sns_logd("config file registry version=%d", atoi(version));
        ret_version = atoi(version);

        if (0 == stat(present_registry_version, &stat_buf)) {
            std::ifstream reg_file_version(present_registry_version);
            std::string regfile_version_str;
            std::getline(reg_file_version, regfile_version_str);

            sns_logi("'%s'(version=%d) Vs '%s'(%s)",
                config_file, atoi(version),
                present_registry_version , regfile_version_str.c_str());

            if (configfile_version_str.compare(regfile_version_str)) {
                version_changed = true;
                sns_loge("sensors registry version changed ['%s'(version=%d) Vs '%s'(%s)]",
                         config_file, atoi(version),
                         present_registry_version , regfile_version_str.c_str());
            }

        } else {
            sns_logi("'%s' is not available", present_registry_version);
            version_changed = false;
        }
    } else {
        sns_logi("config file '%s' do not have version", config_file);
    }
    return version_changed;
}

void sensor_factory::handle_version_update(int version)
{
    /*if registry version changed or didn't exist, parse hw properties again */
    int errors = handle_new_registry_update();

    if(!errors) {
        /* if there were 0 parsing errors, update the registry version in persist */
        std::ofstream reg_file_version_w(present_registry_version);
        reg_file_version_w << "version=" << version << std::endl;
    } else {
        sns_loge("%d errors detected, registry version not updated", errors);
    }
}

void sensor_factory::handle_registry_version_change()
{
    struct stat stat_buf;
    int version = -1;
    /*check for registry version update */
    if (check_registry_version_change(registry_config_file, version)) {
        if (0 == unlink(registry_metadata_file)) {
            sns_logi("registry version is changed, metadata file '%s' deleted",
                        registry_metadata_file);

            /*delete mandatory list as metadata information is removed, so takes time for
              reparsing in slpi so hal wait for more time for sensors list to get them all
              or sometimes new algos or config parameters can be updated and list needs to
              be generated again*/
            if (0 == unlink(mandatory_sensors_list_file))
                sns_loge("registry version is changed, sensorslist file '%s' deleted",
                            mandatory_sensors_list_file);
            else
                sns_loge("failed to delete '%s'", mandatory_sensors_list_file);

            handle_version_update(version);
            _laterboot = false;
        } else if (0 != stat(registry_metadata_file, &stat_buf)) {

            handle_version_update(version);

            sns_logi("'%s' not present , supposed to be firstboot after flash", registry_metadata_file);

        } else {
            sns_loge("failed to delete '%s'", registry_metadata_file);
        }
    } else {
        sns_logi("registry version(%d) is not changed", version);
    }
}
#endif

void sensor_factory::registry_lookup_callback
            (const string& datatype, const std::vector<sensor_uid>& suids)
{
     pthread_mutex_lock(&_m_waitMutex);
     if (suids.size() > 0) {
        _registry_available = true;
     }
     pthread_cond_signal(&_m_waitCond);
     pthread_mutex_unlock(&_m_waitMutex);
}
bool sensor_factory::is_registry_up()
{
    using namespace std::chrono;
    struct timespec ts;
    _registry_available = false;
    std::chrono::steady_clock::time_point tp_start;
    std::chrono::steady_clock::time_point tp_end;

    int retry_attempt_num = REGISTRY_RETRY_CNT;
    sns_logd("starting is_registry_up @ t=%fs",
               duration_cast<duration<double>>(steady_clock::now().time_since_epoch()).count());

    tp_start = steady_clock::now();
    suid_lookup lookup(
        [this](const string& datatype, const auto& suids)
        {
            this->registry_lookup_callback( datatype, suids);
        });
    tp_end = steady_clock::now();

    sns_logi("Remote SensorService UpTime from hal_service start = %fs" ,
             duration_cast<duration<double>>(tp_end - tp_start).count());

    tp_start = steady_clock::now();
    for (; retry_attempt_num > 0; retry_attempt_num--) {
        sns_logd("registry discovery countdown: %d", retry_attempt_num);
        lookup.request_suid("registry", false);
        pthread_mutex_lock(&_m_waitMutex);
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += REGISTRY_UP_WAITTIME_SEC;
        pthread_cond_timedwait(&_m_waitCond, &_m_waitMutex, &ts);
        pthread_mutex_unlock(&_m_waitMutex);
        if (false == _registry_available)
          this_thread::sleep_for(std::chrono::milliseconds(250));
        else
          break;
    }
    tp_end = steady_clock::now();
    if (true == _registry_available)
        sns_logi("registry_sensor availability time '%fsec' from serviceup(no of repeats:'%d') ",
                 duration_cast<duration<double>>(tp_end - tp_start).count(),
                 (REGISTRY_RETRY_CNT-retry_attempt_num));
    else
        sns_loge("registry_sensor unavailable even after '%fsecs' of serviceup(no of repeats:'%d')",
                duration_cast<duration<double>>(tp_end - tp_start).count(),
                (REGISTRY_RETRY_CNT-retry_attempt_num));

	return _registry_available;
}

sensor_factory::sensor_factory()
{
    _settings = get_sns_settings();
    _pending_attributes = 0;
    _laterboot = true;
    _registry_available = false;
    _num_of_non_default_sensors = 0;
    _num_of_response_for_non_default_sensors = 0;
    if (!(_settings & DISABLE_SENSORS_FLAG)) {
        if (true == is_registry_up()) {
            //wait for registry sensor up
#ifdef NO_FIRMWARE_SUPPORT_FOR_PROPERTY_READING
            handle_registry_version_change();
#endif
            init_mandatory_list_db();
            _num_of_non_default_sensors = 0;
            _num_of_response_for_non_default_sensors = 0;

            if (_laterboot) {
                for (const string& s : mandatory_sensor_datatypes) {
                    mandatory_sensors_map.emplace(s, 0);
                }
            }
            /* find available sensors on ssc */
            discover_sensors();

            if (_suid_map.size() > 0) {
                retrieve_attributes();
            }
            /*it enables to check for ssr triggering from HAL*/
            if (ssr_simulate()) {
                sns_logd("simulated slpi ssr  ");
                auto no_sensors = _suid_map.size();
                if (!trigger_ssr()) {
                    discover_sensors();
                if (_suid_map.size() > 0) {
                    retrieve_attributes();
                }
                sns_loge("number of sensors before ssr: %lu",
                    (unsigned long)no_sensors);
                sns_loge("number of sensors after  ssr: %lu",
                    (unsigned long)_suid_map.size());
                } else {
                    sns_loge("ssr trigger failed");
                }
            }
            deinit_mandatory_list_db();
        }   else {
            sns_loge("registry sensor not ready, not trying to get sensors list");
        }
    } else {
        sns_logi("disabled sensors from sns_settings");
    }
}

vector<unique_ptr<sensor>> sensor_factory::get_all_available_sensors() const
{
    vector<unique_ptr<sensor>> all_sensors;
    for (const auto& item : callbacks()) {
        const auto& get_sensors = item.second;
        vector<unique_ptr<sensor>> sensors = get_sensors();
        if(sensors.size() == 0){
          continue;
        }
        sns_logd("type=%d, num_sensors=%u", item.first, (unsigned int)sensors.size());
        for (auto&& s : sensors) {
            if(nullptr != s ) {
              all_sensors.push_back(std::move(s));
            }
        }
    }
    return all_sensors;
}

void sensor_factory::default_suids_lookup_callback(const string& datatype,
                                          const vector<sensor_uid>& suids)
{
    using namespace std::chrono;
    if (suids.size() > 0) {
        lock_guard<mutex> lock(mandatory_sensors_mutex);
        _suid_map[datatype] = suids;
        update_mandatory_list_database(datatype);
        if (_laterboot) {
            auto it = mandatory_sensors_map.find(datatype);
            if (it != mandatory_sensors_map.end() && it->second == false) {
                it->second = true;
                num_mandatory_sensors_found++;
                if (num_mandatory_sensors_found ==
                    mandatory_sensor_datatypes.size()) {
                    all_mandatory_sensors_found.store(true);
                }
            }
        }
        _tp_last_suid = steady_clock::now();
        sns_logv("received suid for dt=%s, t=%fs", datatype.c_str(),
                 duration_cast<duration<double>>(
                     _tp_last_suid.time_since_epoch()).count());
    }
}

void sensor_factory::all_suids_lookup_callback(const string& datatype,
                                          const vector<sensor_uid>& suids)
{
    using namespace std::chrono;
    if (suids.size() > 0) {
        lock_guard<mutex> lock(mandatory_sensors_mutex);
        _all_suids_container[datatype] = suids;
        _num_of_response_for_non_default_sensors++;
        _tp_last_suid = steady_clock::now();
        sns_logv("received all suid for dt=%s, t=%fs, count = %d", datatype.c_str(),
                 duration_cast<duration<double>>(
                     _tp_last_suid.time_since_epoch()).count(), _num_of_response_for_non_default_sensors);
        _cv_non_default_sensor.notify_one();
    }
}


void sensor_factory::wait_for_sensors(suid_lookup& lookup)
{
    int count = MANDATORY_SENSOR_WAIT_TIME_SEC;
    while (count) {
        sns_logi("SEE not ready yet wait count %d", count);
        {
            for (auto& dt : datatypes()) {
                if (_suid_map.find(dt.first) == _suid_map.end()) {
                    sns_logd("re-sending request for %s", dt.first.c_str());
                    lookup.request_suid(dt.first, true);
                }
            }
        }

        this_thread::sleep_for(std::chrono::milliseconds(2*MSEC_PER_SEC));
        count--;
        {
            lock_guard<mutex> lock(mandatory_sensors_mutex);
            if (num_mandatory_sensors_found) {
                sns_logi("sensors found after re discover %d", count);
                break;
            }
            if ((count == 0) && (num_mandatory_sensors_found == 0)) {
                sns_loge("sensors list is 0 after first boot ");
                break;
            }
        }
    }
}

void sensor_factory::wait_for_mandatory_sensors(suid_lookup& lookup)
{
    int count = MANDATORY_SENSOR_WAIT_TIME_SEC;
    while (all_mandatory_sensors_found.load() == false) {
        sns_logi("some mandatory sensors not available yet, "
                 "will wait for %d seconds...", count);
        {
            std::unique_lock<mutex> guard(mandatory_sensors_mutex);
            for (auto p : mandatory_sensors_map) {
                if (p.second == false) {
                    sns_logd("non available ones %s", p.first.c_str());
                }
            }
            guard.unlock();
            for (auto& dt : datatypes()) {
                if (_suid_map.find(dt.first) == _suid_map.end()) {
                    sns_logd("re-sending request for %s", dt.first.c_str());
                    lookup.request_suid(dt.first, true);
                }
            }
        }

        this_thread::sleep_for(std::chrono::milliseconds(MSEC_PER_SEC));
        count--;

        {
            lock_guard<mutex> lock(mandatory_sensors_mutex);
            if (count == 0 && all_mandatory_sensors_found.load() == false) {
                sns_loge("some mandatory sensors not available even after %d "
                         "seconds, giving up.", MANDATORY_SENSOR_WAIT_TIME_SEC);
                sns_loge("%lu missing sensor(s)", (unsigned long)
                         mandatory_sensor_datatypes.size() - num_mandatory_sensors_found);
                for (auto p : mandatory_sensors_map) {
                    if (p.second == false) {
                        sns_loge("    %s", p.first.c_str());
                    }
                }
                break;
            }
        }
    }
}

void sensor_factory::discover_sensors()
{
    using namespace std::chrono;

    sns_logd("discovery start t=%fs", duration_cast<duration<double>>(
             steady_clock::now().time_since_epoch()).count());

    suid_lookup lookup(
        [this](const string& datatype, const auto& suids)
        {
            this->default_suids_lookup_callback(datatype, suids);
        });

    sns_logd("discovering available sensors...");

    for (const auto& dt : datatypes()) {
        sns_logd("requesting %s", dt.first.c_str());
        lookup.request_suid(dt.first, true);
    }
    auto tp_wait_start = steady_clock::now();

    /* wait for some time for discovery of available sensors */
    auto delay = get_discovery_timeout_ms();

    sns_logv("before sleep, now=%f", duration_cast<duration<double>>(
             steady_clock::now().time_since_epoch()).count());

    sns_logi("waiting for suids for %us ...", (delay/MSEC_PER_SEC));

    this_thread::sleep_for(std::chrono::milliseconds(delay));

    sns_logv("after sleep, now=%f", duration_cast<duration<double>>(
             steady_clock::now().time_since_epoch()).count());

    /* additional wait for discovery of critical sensors */
    if (_laterboot) {
        wait_for_mandatory_sensors(lookup);
    } else if (0 == num_mandatory_sensors_found) {
        sns_loge("first boot generated 0 sensors so re-discover");
        wait_for_sensors(lookup);
    }
    sns_logd("available sensors on ssc");
    for (const auto& item : _suid_map) {
        sns_logd("%-20s%4u", item.first.c_str(), (unsigned int)item.second.size());
    }

    sns_logd("start discovering non-default sensors");
    for (const auto& dt : datatypes())  {
#ifdef ALLOW_NON_DEFAULT_DUAL_SENSOR_DISCOVERY
        // To avoid unnecessary wating time, check the datatype requested for non-default
        // was already discovered during default sensor look-up process
        if (dt.second == false) {
            _num_of_non_default_sensors++;
            if (_suid_map.find(dt.first) == _suid_map.end()) {
                sns_logi("the default sensor of %s was not discovered", dt.first.c_str());
            }
        }
#else
        if (dt.second == false && (_suid_map.find(dt.first) != _suid_map.end())) {
            _num_of_non_default_sensors++;
        }
#endif
    }
    sns_logd("num of non-default sensor request = %d", _num_of_non_default_sensors);
    if (_num_of_non_default_sensors > 0) {
        std::chrono::steady_clock::time_point nd_disc_start = steady_clock::now();
        suid_lookup lookup_all(
            [this](const string& datatype, const auto& suids)
            {
                this->all_suids_lookup_callback(datatype, suids);
            });
        std::unique_lock<std::mutex> lk(mandatory_sensors_mutex);
        for (const auto& dt : datatypes()) {
            if (dt.second == false) {
                sns_logd("requesting non-default sensors for %s", dt.first.c_str());
                lookup_all.request_suid(dt.first, false);
            }
        }
        bool timeout = _cv_non_default_sensor.wait_for(lk, std::chrono::milliseconds(NON_DEFAULT_SENSOR_WAIT_TIME_MSEC),
            [this]{
               bool result = (_num_of_response_for_non_default_sensors == _num_of_non_default_sensors? true: false);
               return result;
            });
        _default_suids_container = _suid_map;
        for(auto& item : _suid_map) {
            auto it =_all_suids_container.find(item.first);
            if (it != _all_suids_container.end()) {
                //replace the exsiting suids with the one from _all_suids_container
                item.second = it->second;
            }
        }
#ifdef ALLOW_NON_DEFAULT_DUAL_SENSOR_DISCOVERY
        for(auto& item : _all_suids_container) {
            auto it = _suid_map.find(item.first);
            if (it == _suid_map.end()) {
                sns_logi("add %s sensor which is non-default", item.first.c_str());
                _suid_map[item.first] = item.second;
            }
        }
#endif
        sns_logi("not-default suid discovery time = %fs",
             duration_cast<duration<double>>(steady_clock::now() - nd_disc_start)
             .count());

    } else {
        _default_suids_container = _suid_map;
    }
    sns_logi("suid discovery time = %fs",
             duration_cast<duration<double>>(_tp_last_suid - tp_wait_start)
             .count());
}

sns_client_request_msg sensor_factory::create_attr_request(sensor_uid suid)
{
    sns_client_request_msg pb_req_msg;
    sns_std_attr_req pb_attr_req;
    string pb_attr_req_encoded;
    /* populate the pb_attr_req message */
    pb_attr_req.set_register_updates(false);
    pb_attr_req.SerializeToString(&pb_attr_req_encoded);
    /* populate the client request message */
    pb_req_msg.set_msg_id(SNS_STD_MSGID_SNS_STD_ATTR_REQ);
    pb_req_msg.mutable_request()->set_payload(pb_attr_req_encoded);
    pb_req_msg.mutable_suid()->set_suid_high(suid.high);
    pb_req_msg.mutable_suid()->set_suid_low(suid.low);
    pb_req_msg.mutable_susp_config()->set_delivery_type(SNS_CLIENT_DELIVERY_WAKEUP);
    pb_req_msg.mutable_susp_config()->set_client_proc_type(SNS_STD_CLIENT_PROCESSOR_APSS);

    return pb_req_msg;
}

void sensor_factory::retrieve_attributes()
{
    ssc_connection ssc_conn([this](const uint8_t* data, size_t size, uint64_t ts)
    {
        sns_client_event_msg pb_event_msg;
        pb_event_msg.ParseFromArray(data, size);
        for (int i=0; i < pb_event_msg.events_size(); i++) {
            auto&& pb_event = pb_event_msg.events(i);
            sns_logv("event[%d] msg_id=%d", i, pb_event.msg_id());
            if (pb_event.msg_id() ==  SNS_STD_MSGID_SNS_STD_ATTR_EVENT) {
                sensor_uid suid (pb_event_msg.suid().suid_low(),
                                 pb_event_msg.suid().suid_high());
                handle_attribute_event(suid, pb_event);
            }
        }
    });
    ssc_conn.set_worker_name("see_attr_cb");
    /* send attribute request for all suids */
    for (const auto& item : _suid_map) {
        const auto& datatype = item.first;
        const auto& suids = item.second;
        int count=0;
        for (const sensor_uid& suid : suids) {
            _attr_mutex.lock();
            _pending_attributes++;
            _attr_mutex.unlock();
            string pb_attr_req_encoded;
            sns_logd("requesting attributes for %s-%d", datatype.c_str(),
                     count++);
            create_attr_request(suid).SerializeToString(&pb_attr_req_encoded);
            ssc_conn.send_request(pb_attr_req_encoded);
        }
    }

    sns_logd("waiting for attributes...");

    /* wait until attributes are received */
    int count = GET_ATTR_NUM_ITERS;
    uint64_t delay = SENSOR_GET_ATTR_TIMEOUT_NS / GET_ATTR_NUM_ITERS;
    struct timespec ts;
    int err;
    ts.tv_sec = delay / NSEC_PER_SEC;
    ts.tv_nsec = delay % NSEC_PER_SEC;

    unique_lock<mutex> lk(_attr_mutex);
    while (count > 0) {
        if (_pending_attributes == 0) {
            break;
        }
        /* unlock the mutex while sleeping */
        lk.unlock();
        do {
            err = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        } while (err == EINTR);
        lk.lock();
        if (err != 0) {
            sns_loge("clock_nanosleep() failed, err=%d, count=%d", err, count);
        }
        count--;
    }

    if (_pending_attributes > 0) {
        sns_loge("timeout while waiting for attributes, pending = %d, err = %d",
              _pending_attributes, err);
        return;
    }

    sns_logd("attributes received");
}

void sensor_factory::handle_attribute_event(sensor_uid suid,
    const sns_client_event_msg_sns_client_event& pb_event)
{
    sns_std_attr_event pb_attr_event;
    pb_attr_event.ParseFromString(pb_event.payload());

    sensor_attributes attr;

    for (int i=0; i<pb_attr_event.attributes_size(); i++) {
        attr.set(pb_attr_event.attributes(i).attr_id(),
                       pb_attr_event.attributes(i).value());
    }
    _attributes[suid] = attr;

    /* notify the thread waiting for attributes */
    unique_lock<mutex> lock(_attr_mutex);
    if (_pending_attributes > 0) {
        _pending_attributes--;
    }
}

int sensor_factory::get_pairedsuid(
                               const std::string& datatype,
                               const sensor_uid &master_suid,
                               sensor_uid &paired_suid)
{
    /*get the list of suids */
    std::vector<sensor_uid>  suids = get_suids(datatype);
    const string master_suid_vendor = _attributes[master_suid].get_string(SNS_STD_SENSOR_ATTRID_VENDOR);
    const string master_suid_name   = _attributes[master_suid].get_string(SNS_STD_SENSOR_ATTRID_NAME);
    const string master_type = _attributes[master_suid].get_string(SNS_STD_SENSOR_ATTRID_TYPE);
    int master_suid_hwid = -1;
    bool match = false;
    if (_attributes[master_suid].is_present(SNS_STD_SENSOR_ATTRID_HW_ID)) {
        master_suid_hwid = _attributes[master_suid].get_ints(SNS_STD_SENSOR_ATTRID_HW_ID)[0];
    }

    sns_logd("to be paired::datatype: %s, vendor: %s , name:%s, hwid %d",
                datatype.c_str(), master_suid_vendor.c_str(),
                master_suid_name.c_str(), master_suid_hwid);

    /*get attributes of specified data type suid & return if matches*/
    for (std::vector<sensor_uid>::iterator it = suids.begin(); it != suids.end(); ++it) {
        sns_logv("for pairing.., parsed vendor:%s, name:%s ",
                    _attributes[*it].get_string(SNS_STD_SENSOR_ATTRID_VENDOR).c_str(),
                    _attributes[*it].get_string(SNS_STD_SENSOR_ATTRID_NAME).c_str());

        if ((_attributes[*it].get_string(SNS_STD_SENSOR_ATTRID_VENDOR)
                 == master_suid_vendor ) &&
                 (_attributes[*it].get_string(SNS_STD_SENSOR_ATTRID_NAME)
                 ==(master_suid_name))) {
            match = true;
            /*HW_ID is optional field so check if it is present only*/
            if ( master_suid_hwid != -1 && _attributes[*it].is_present(SNS_STD_SENSOR_ATTRID_HW_ID)){
                if ( master_suid_hwid != _attributes[*it].get_ints(SNS_STD_SENSOR_ATTRID_HW_ID)[0]) {
                    match = false;
                    continue;
                }
            }
            if( match == true) {
                paired_suid = *it;
                sns_logd("datatype: %s, paired with:type:%s,name:%s,vendor:%s",
                            datatype.c_str(), master_type.c_str(),
                            _attributes[*it].get_string(SNS_STD_SENSOR_ATTRID_NAME).c_str(),
                            _attributes[*it].get_string(SNS_STD_SENSOR_ATTRID_VENDOR).c_str());
                break;
            }
        }
    }

    if(match == false) {
       sns_loge("no data type mapping b/w master/paired %s/%s",
                                        master_type.c_str(), datatype.c_str());
       return -1;
    }

    return 0;
}

int sensor_factory::get_pairedcalsuid(
                               const std::string& datatype,
                               const sensor_uid &master_suid,
                               sensor_uid &paired_suid)
{
    /*get the list of suids */
    std::vector<sensor_uid>  suids = get_suids(datatype);
    int rigid_body = -1;
    bool match = false;

    if (!_attributes[master_suid].is_present(SNS_STD_SENSOR_ATTRID_RIGID_BODY)) {
        sns_loge("rigid_body attribute is not available for suid: %" PRIu64 "%" PRIu64,
            master_suid.low, master_suid.high);
        return -1;
    }
    rigid_body = _attributes[master_suid].get_ints(SNS_STD_SENSOR_ATTRID_RIGID_BODY)[0];

    sns_logd("to be paired::datatype: %s, rigid_body: %d",
                datatype.c_str(), rigid_body);

    /*get attributes of specified data type suid & return if matches*/
    for (std::vector<sensor_uid>::iterator it = suids.begin(); it != suids.end(); ++it) {
        sensor_uid temp = *it;
        if (!_attributes[*it].is_present(SNS_STD_SENSOR_ATTRID_RIGID_BODY)) {
          sns_loge("rigid body attribute not available for suid: %" PRIu64 "%" PRIu64,
              temp.low, temp.high);
          continue;
        }

        sns_logv("for pairing.., parsed rigid_body = %" PRId64,
                    _attributes[*it].get_ints(SNS_STD_SENSOR_ATTRID_RIGID_BODY)[0]);

        if (_attributes[*it].get_ints(SNS_STD_SENSOR_ATTRID_RIGID_BODY)[0]
                 == rigid_body ) {
            match = true;
            paired_suid = *it;
            sns_logd("datatype: %s, paired with:rigid_body = %d",
                       datatype.c_str(), rigid_body);
            break;
        }
    }

    if(match == false) {
       sns_loge("no data type mapping b/w master/paired %d/%s",
                                        rigid_body, datatype.c_str());
       return -1;
    }

    return 0;
}



const std::vector<sensor_uid>& sensor_factory::get_suids(const std::string& datatype) const
{
    auto it = _suid_map.find(datatype);
    if (it != _suid_map.end()) {
        return it->second;
    } else {
        static vector<sensor_uid> empty;
        return empty;
    }
}

bool sensor_factory::is_default_sensor(const std::string& datatype, const sensor_uid& suid)
{
    auto it = _default_suids_container.find(datatype);
    if (it != _default_suids_container.end()) {
        const auto& suids = it->second;
        for(const sensor_uid& default_suid : suids) {
            if (suid == default_suid) {
                return true;
            }
        }
    }
    return false;
}
