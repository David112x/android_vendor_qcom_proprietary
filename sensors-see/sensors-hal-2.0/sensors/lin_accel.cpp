/*
 * Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <cinttypes>
#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sns_gravity.pb.h"

static const char *SSC_DATATYPE_LINEAR_ACCELERATION = "gravity";
static const uint32_t LINEAR_ACCELERATION_RESERVED_FIFO_COUNT = 300;

class lin_accel : public ssc_sensor
{
public:
    lin_accel(sensor_uid suid, sensor_wakeup_type wakeup);
    static const char* ssc_datatype() { return SSC_DATATYPE_LINEAR_ACCELERATION; }

private:
    virtual void handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event) override;
};

lin_accel::lin_accel(sensor_uid suid, sensor_wakeup_type wakeup):
    ssc_sensor(suid, wakeup)
{
    set_type(SENSOR_TYPE_LINEAR_ACCELERATION);
    set_string_type(SENSOR_STRING_TYPE_LINEAR_ACCELERATION);
    set_fifo_reserved_count(LINEAR_ACCELERATION_RESERVED_FIFO_COUNT);
    if (wakeup)
        set_name("linear_acceleration_wakeup");
    else
        set_name("linear_acceleration");
}

void lin_accel::handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event)
{
    if (SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT == pb_event.msg_id()) {
        sns_std_sensor_event pb_gravity_event;
        pb_gravity_event.ParseFromString(pb_event.payload());

        if(6 == pb_gravity_event.data_size()) {
            Event hal_event =
                create_sensor_hal_event(pb_event.timestamp());

            hal_event.u.vec3.x = pb_gravity_event.data(3);
            hal_event.u.vec3.y = pb_gravity_event.data(4);
            hal_event.u.vec3.z = pb_gravity_event.data(5);
            hal_event.u.vec3.status = (SensorStatus)pb_gravity_event.status();

            cal_sample_inerval(pb_event.timestamp());
            calculate_latency(hal_event.timestamp);
            cal_slpi_latency(pb_event.timestamp());

            if(true == can_submit_sample(hal_event))
              events.push_back(hal_event);

            sns_logv("lin_accel_event: ts=%" PRIi64 ", (%f, %f, %f)",
                    hal_event.timestamp,
                    hal_event.u.vec3.x,
                    hal_event.u.vec3.y,
                    hal_event.u.vec3.z);
        } else {
            sns_loge("Invalid lin_accel event");
        }
    }
}

static bool lin_accel_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(SENSOR_TYPE_LINEAR_ACCELERATION,
        ssc_sensor::get_available_sensors<lin_accel>);
    sensor_factory::request_datatype(SSC_DATATYPE_LINEAR_ACCELERATION);
    return true;
}

SENSOR_MODULE_INIT(lin_accel_module_init);
