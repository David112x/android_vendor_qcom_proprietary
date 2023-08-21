/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "additionalinfo_sensor.h"

/*get the handle and type from parent*/
additionalinfo_sensor::additionalinfo_sensor(sensor_uid suid,
                                sensor_wakeup_type wakeup,
                                int handle_parentsensor,
                                bool process_in_qmicallback,
                                uint32_t atrace_delay_ms):
    ssc_sensor(suid, wakeup)
{
    set_type(SENSOR_TYPE_ADDITIONAL_INFO);
    set_string_type(SENSOR_STRING_TYPE_ADDITIONAL_INFO);
    set_resampling(true);
    donot_honor_flushevent(true);
    _handle_parentsensor = handle_parentsensor;
    update_system_prop_details(process_in_qmicallback, atrace_delay_ms);

}

void additionalinfo_sensor::send_additional_info(int64_t timestamp)
{

    Event hal_event;
    hal_event.sensorType = (SensorType)SENSOR_TYPE_ADDITIONAL_INFO;
    sns_logd("gyro_temp_additional_info temp=%f, ts=%lld", _temp,
             (long long) timestamp);
    hal_event.sensorHandle =_handle_parentsensor;

    /* additional_info frame begin */
    hal_event.timestamp = timestamp;
    hal_event.u.additional.type =
        (::android::hardware::sensors::V1_0::AdditionalInfoType)AINFO_BEGIN;
    events.push_back(hal_event);

    hal_event.timestamp++;
    hal_event.u.additional.type =
            (::android::hardware::sensors::V1_0::AdditionalInfoType)AINFO_INTERNAL_TEMPERATURE;
    hal_event.u.additional.u.data_float[0] = _temp;
    events.push_back(hal_event);


    /* additional_info frame end */
    hal_event.timestamp++;
    hal_event.u.additional.type =
        (::android::hardware::sensors::V1_0::AdditionalInfoType)AINFO_END;
    events.push_back(hal_event);
}

void additionalinfo_sensor::handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event)
{
    if (SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_EVENT == pb_event.msg_id()) {
        sns_std_sensor_event pb_sensor_event;
        pb_sensor_event.ParseFromString(pb_event.payload());

        if(pb_sensor_event.data_size()) {
            int64_t timestamp = pb_event.timestamp();
            sns_logv("datatype.c_str() %s", datatype().c_str());
            if (!strncmp("sensor_temperature", datatype().c_str(), sizeof("sensor_temperature"))) {
                _temp = pb_sensor_event.data(0);
                send_additional_info(timestamp);
            }
        } else {
            sns_loge("empty data returned for sensor temperature");
        }
    }
}

