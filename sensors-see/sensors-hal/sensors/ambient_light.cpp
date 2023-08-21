/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sns_std_sensor.pb.h"

using namespace std;

static const char *SSC_DATATYPE_AMBIENT_LIGHT = "ambient_light";

class ambient_light : public ssc_sensor
{
public:
    ambient_light(sensor_uid suid, sensor_wakeup_type wakeup);
    static const char* ssc_datatype() { return SSC_DATATYPE_AMBIENT_LIGHT; }

private:
    virtual void handle_sns_std_sensor_event(
        const sns_client_event_msg_sns_client_event& pb_event) override;
};

ambient_light::ambient_light(sensor_uid suid, sensor_wakeup_type wakeup):
    ssc_sensor(suid, wakeup)
{
    set_type(SENSOR_TYPE_LIGHT);
    set_string_type(SENSOR_STRING_TYPE_LIGHT);
    set_sensor_typename("Ambient Light Sensor");
    set_nowk_msgid(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT);
}

void ambient_light::handle_sns_std_sensor_event(
        const sns_client_event_msg_sns_client_event& pb_event)
{
    sns_std_sensor_event pb_sensor_event;
    pb_sensor_event.ParseFromString(pb_event.payload());

    if (pb_sensor_event.status() != SNS_STD_SENSOR_SAMPLE_STATUS_ACCURACY_HIGH) {
        sns_logi("ambient_light_event: filter out status = %d, lux = %lu, raw_adc=%lu",
                 pb_sensor_event.status(),
                 (unsigned long)pb_sensor_event.data(0),
                 (unsigned long)pb_sensor_event.data(1));
        return;
    }
    if (pb_sensor_event.data_size() < 2) {
        sns_loge("ambient_light_event: wrong event");
        return;
    }
    sensors_event_t hal_event = create_sensor_hal_event(pb_event.timestamp());
    hal_event.data[0] = pb_sensor_event.data(0);
    hal_event.data[1] = pb_sensor_event.data(1);

    if (!duplicate_onchange_sample(hal_event))
        submit_sensors_hal_event(hal_event);

    sns_logv("ambient_light_event: ts=%llu, lux = %lu, raw_adc=%lu",
             (unsigned long long)hal_event.timestamp,
             (unsigned long)pb_sensor_event.data(0),
             (unsigned long)pb_sensor_event.data(1));
}

static vector<unique_ptr<sensor>> get_available_als_sensors()
{
    const vector<sensor_uid>& als_suids =
         sensor_factory::instance().get_suids(SSC_DATATYPE_AMBIENT_LIGHT);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : als_suids) {
        const sensor_attributes& attr =
            sensor_factory::instance().get_attributes(suid);
        /* publish only on-change proximity sensor */
        if (attr.get_stream_type() != SNS_STD_SENSOR_STREAM_TYPE_ON_CHANGE) {
            continue;
        }
        try {
            sensors.push_back(make_unique<ambient_light>(suid, SENSOR_WAKEUP));
        } catch (const exception& e) {
            sns_loge("failed for wakeup, %s", e.what());
        }
        try {
            sensors.push_back(make_unique<ambient_light>(suid, SENSOR_NO_WAKEUP));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}

static bool ambient_light_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(
        SENSOR_TYPE_LIGHT, get_available_als_sensors);
    sensor_factory::request_datatype(SSC_DATATYPE_AMBIENT_LIGHT);
    return true;
}

SENSOR_MODULE_INIT(ambient_light_module_init);
