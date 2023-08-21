/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <cinttypes>
#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sns_tilt.pb.h"

static const char *SSC_DATATYPE_TILT_DETECTOR = "tilt";

class tilt : public ssc_sensor
{
public:
    tilt(sensor_uid suid, sensor_wakeup_type wakeup);
    static const char* ssc_datatype() { return SSC_DATATYPE_TILT_DETECTOR; }

private:
    virtual void handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event) override;
};

tilt::tilt(sensor_uid suid, sensor_wakeup_type wakeup):
    ssc_sensor(suid, wakeup)
{
    set_type(SENSOR_TYPE_TILT_DETECTOR);
    set_string_type(SENSOR_STRING_TYPE_TILT_DETECTOR);
    set_reporting_mode(SENSOR_FLAG_SPECIAL_REPORTING_MODE);
    if(false == is_resolution_set)
        set_resolution(1.0f);
    if(false == is_max_range_set)
        set_max_range(1.0f);
}

void tilt::handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event)
{
    if (SNS_TILT_MSGID_SNS_TILT_EVENT == pb_event.msg_id()) {
        Event hal_event =
             create_sensor_hal_event(pb_event.timestamp());

        hal_event.u.scalar = 1.0f;
        if(true == can_submit_sample(hal_event))
          events.push_back(hal_event);

        sns_logv("tilt_event: ts=%" PRIi64, hal_event.timestamp);
    }
}

static bool tilt_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(SENSOR_TYPE_TILT_DETECTOR,
        ssc_sensor::get_available_wakeup_sensors<tilt>);
    sensor_factory::request_datatype(SSC_DATATYPE_TILT_DETECTOR);
    return true;
}

SENSOR_MODULE_INIT(tilt_module_init);

