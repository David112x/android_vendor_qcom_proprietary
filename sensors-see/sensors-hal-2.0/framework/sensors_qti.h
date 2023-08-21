/*
 * Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#pragma once

/* All QTI defined sensor types must start from QTI_SENSOR_TYPE_BASE
   This is to ensure no collission with the Android sensor types
   defined in sensors.h */
#define QTI_SENSOR_TYPE_BASE 33171000

#define QTI_DUAL_SENSOR_TYPE_BASE 268369920


#define QTI_SENSOR_TYPE_RGB                     (QTI_SENSOR_TYPE_BASE + 1)
#define QTI_SENSOR_STRING_TYPE_RGB              "qti.sensor.rgb"

#define QTI_SENSOR_TYPE_HALL_EFFECT             (QTI_SENSOR_TYPE_BASE + 2)
#define QTI_SENSOR_STRING_TYPE_HALL_EFFECT      "qti.sensor.hall_effect"

#define QTI_SENSOR_TYPE_ULTRAVIOLET             (QTI_SENSOR_TYPE_BASE + 3)
#define QTI_SENSOR_STRING_TYPE_ULTRAVIOLET      "qti.sensor.ultra_violet"

#define QTI_SENSOR_TYPE_THERMOPILE              (QTI_SENSOR_TYPE_BASE + 4)
#define QTI_SENSOR_STRING_TYPE_THERMOPILE       "qti.sensor.thermopile"

#define QTI_SENSOR_TYPE_SAR                     (QTI_SENSOR_TYPE_BASE + 5)
#define QTI_SENSOR_STRING_TYPE_SAR              "qti.sensor.sar"

#define QTI_SENSOR_TYPE_ACCELEROMETER                      (QTI_DUAL_SENSOR_TYPE_BASE + SENSOR_TYPE_ACCELEROMETER)
#define QTI_SENSOR_STRING_TYPE_ACCELEROMETER               "qti.sensor.accel"

#define QTI_SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED         (QTI_DUAL_SENSOR_TYPE_BASE + SENSOR_TYPE_ACCELEROMETER_UNCALIBRATED)
#define QTI_SENSOR_STRING_TYPE_ACCELEROMETER_UNCALIBRATED  "qti.sensor.accel-uncalibrated"

#define QTI_SENSOR_TYPE_GYROSCOPE                          (QTI_DUAL_SENSOR_TYPE_BASE + SENSOR_TYPE_GYROSCOPE)
#define QTI_SENSOR_STRING_TYPE_GYROSCOPE                   "qti.sensor.gyroscope"

#define QTI_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED             (QTI_DUAL_SENSOR_TYPE_BASE + SENSOR_TYPE_GYROSCOPE_UNCALIBRATED)
#define QTI_SENSOR_STRING_TYPE_GYROSCOPE_UNCALIBRATED      "qti.sensor.gyroscope-uncalibrated"
