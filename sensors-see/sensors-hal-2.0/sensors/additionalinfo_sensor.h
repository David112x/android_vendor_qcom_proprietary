/*
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include "ssc_sensor.h"



class additionalinfo_sensor : public ssc_sensor {

public:
    additionalinfo_sensor(sensor_uid suid,
                    sensor_wakeup_type wakeup,
                    int handle_parentsensor,
                    bool process_in_qmicallback,
                    uint32_t atrace_delay_ms);

    /* send gyro_temp additional info to hal */
    void send_additional_info(int64_t timestamp);

private:
    virtual void handle_sns_client_event(
        const sns_client_event_msg_sns_client_event& pb_event) override;
    int _handle_parentsensor;
    float _temp;
};
