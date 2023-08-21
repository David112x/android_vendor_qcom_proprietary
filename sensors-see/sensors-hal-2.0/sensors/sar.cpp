/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sensors_qti.h"
#include "sns_sar.pb.h"

using namespace std;

static const char *SSC_DATATYPE_SAR = "sar";
static const uint32_t SAR_RESERVED_FIFO_COUNT = 300;

using namespace std;

class sar : public ssc_sensor
{
public:
    sar(sensor_uid suid, sensor_wakeup_type wakeup);
    static const char* ssc_datatype() { return SSC_DATATYPE_SAR; }

private:
    virtual void handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event) override;
};

sar::sar(sensor_uid suid, sensor_wakeup_type wakeup):
    ssc_sensor(suid, wakeup)
{
    set_type(QTI_SENSOR_TYPE_SAR);
    set_string_type(QTI_SENSOR_STRING_TYPE_SAR);
    set_fifo_reserved_count(SAR_RESERVED_FIFO_COUNT);
    set_nowk_msgid( SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT);
    set_sensor_typename("SAR Sensor");
}

void sar::handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event)
{
    if (pb_event.msg_id() == SNS_SAR_MSGID_SNS_SAR_EVENT) {
        sns_sar_event pb_sar_event;
        pb_sar_event.ParseFromString(pb_event.payload());

        Event hal_event =
             create_sensor_hal_event(pb_event.timestamp());

        if (pb_sar_event.sar_event_type() ==
                SNS_SAR_EVENT_TYPE_NEAR) {
            hal_event.u.scalar = 0.0f;
        } else {
            hal_event.u.scalar = get_sensor_info().maxRange;
        }

        if(true == can_submit_sample(hal_event))
          events.push_back(hal_event);

        sns_logv("sar_event: ts=%llu, near_far=%d, distance=%f",
                 (unsigned long long)hal_event.timestamp,
                 pb_sar_event.sar_event_type(),
                 hal_event.u.scalar);
    }
}

static vector<unique_ptr<sensor>> get_available_sar_sensors()
{
    const vector<sensor_uid>& suids =
         sensor_factory::instance().get_suids(SSC_DATATYPE_SAR);
    vector<unique_ptr<sensor>> sensors;
    for (const auto& suid : suids) {
        const sensor_attributes& attr =
            sensor_factory::instance().get_attributes(suid);
        /* publish only on-change sar sensor */
        if (attr.get_stream_type() != SNS_STD_SENSOR_STREAM_TYPE_ON_CHANGE) {
            continue;
        }
        try {
            sensors.push_back(make_unique<sar>(suid, SENSOR_WAKEUP));
        } catch (const exception& e) {
            sns_loge("failed for wakeup, %s", e.what());
        }
        try {
            sensors.push_back(make_unique<sar>(suid, SENSOR_NO_WAKEUP));
        } catch (const exception& e) {
            sns_loge("failed for nowakeup, %s", e.what());
        }
    }
    return sensors;
}

static bool sar_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(QTI_SENSOR_TYPE_SAR,
                                     get_available_sar_sensors);
    sensor_factory::request_datatype(SSC_DATATYPE_SAR);
    return true;
}

SENSOR_MODULE_INIT(sar_module_init);
