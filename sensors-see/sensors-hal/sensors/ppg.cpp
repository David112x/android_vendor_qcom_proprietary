/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "sensor_factory.h"
#include "ssc_sensor.h"
#include "sns_std_sensor.pb.h"
#include "sns_physical_sensor_test.pb.h"
#include "sensors_qti.h"

#define SENSOR_TYPE_PPG                         65572
#define SENSOR_STRING_TYPE_PPG                  "com.google.wear.sensor.ppg"

static const char *SSC_DATATYPE_PPG = "ppg";
static const uint32_t PPG_RESERVED_FIFO_COUNT = 300;

class ppg : public ssc_sensor
{
public:
    ppg(sensor_uid suid, sensor_wakeup_type wakeup);
    static const char* ssc_datatype() { return SSC_DATATYPE_PPG; }
};

ppg::ppg(sensor_uid suid, sensor_wakeup_type wakeup):
    ssc_sensor(suid, wakeup)
{
    set_type(SENSOR_TYPE_PPG);
    set_string_type(SENSOR_STRING_TYPE_PPG);
    set_sensor_typename("PPG Sensor");
    set_fifo_reserved_count(PPG_RESERVED_FIFO_COUNT);
    set_nowk_msgid(SNS_STD_SENSOR_MSGID_SNS_STD_SENSOR_PHYSICAL_CONFIG_EVENT);
}

static bool ppg_module_init()
{
    /* register supported sensor types with factory */
    sensor_factory::register_sensor(SENSOR_TYPE_PPG,
        ssc_sensor::get_available_sensors<ppg>);
    sensor_factory::request_datatype(SSC_DATATYPE_PPG);
    return true;
}

SENSOR_MODULE_INIT(ppg_module_init);

