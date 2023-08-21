# Makefile.am - Automake script for Sensors-hal

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -Wno-unused-variable \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./framework/ \
              -I./sensors/ \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc/proto \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-diag-log/inc

cpp_sources = ./sensors/accelerometer.cpp \
              ./sensors/additionalinfo_sensor.cpp \
              ./sensors/ambient_light.cpp \
              ./sensors/ambient_temperature.cpp \
              ./sensors/device_orient.cpp \
              ./sensors/game_rv.cpp \
              ./sensors/geomag_rv.cpp \
              ./sensors/gravity.cpp \
              ./sensors/gyroscope.cpp \
              ./sensors/hall_effect.cpp \
              ./sensors/heart_beat.cpp \
              ./sensors/heart_rate.cpp \
              ./sensors/humidity.cpp \
              ./sensors/lin_accel.cpp \
              ./sensors/magnetometer.cpp \
              ./sensors/motion_detect.cpp \
              ./sensors/offbody_detect.cpp \
              ./sensors/pose_6dof.cpp \
              ./sensors/pressure.cpp \
              ./sensors/proximity.cpp \
              ./sensors/rgb.cpp \
              ./sensors/rotv.cpp \
              ./sensors/sig_motion.cpp \
              ./sensors/stationary_detect.cpp \
              ./sensors/step_count.cpp \
              ./sensors/step_detect.cpp \
              ./sensors/thermopile.cpp \
              ./sensors/tilt.cpp \
              ./sensors/ultraviolet.cpp \
              ./sensors/wrist_tilt.cpp \
              ./framework/direct_channel.cpp \
              ./framework/rpcmem_android.c \
              ./framework/sensor_attributes.cpp \
              ./framework/sensor.cpp \
              ./framework/sensor_factory.cpp \
              ./framework/sensors_hal.cpp \
              ./framework/sensors_hw_module.cpp \
              ./framework/ssc_sensor.cpp

requiredlibs = -lsensorslog -lsnsproto -lssc -lsnsdiaglog -ldl -llog -lcutils $(DIAG_LIBS)

library_includedir = $(pkgincludedir)

include_HEADERS = ./framework/concurrent_queue.h \
                  ./framework/direct_channel.h \
                  ./framework/sensor_attributes.h \
                  ./framework/sensor_factory.h \
                  ./framework/sensor.h \
                  ./framework/sensors.h \
                  ./framework/sensors-base.h \
                  ./framework/sensors_hal_common.h \
                  ./framework/sensors_hal.h \
                  ./framework/sensors_qti.h \
                  ./framework/ssc_sensor.h

lib_LTLIBRARIES = libsensors.ssc.la
libsensors_ssc_la_CC = @CC@
libsensors_ssc_la_SOURCES = $(cpp_sources)
libsensors_ssc_la_CPPFLAGS = $(AM_CPPFLAGS) @DIAG_CFLAGS@
libsensors_ssc_la_LDFLAGS = -shared -version-info 1:0:0
if USE_GLIB
 libsensors_ssc_la_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 libsensors_ssc_la_LDFLAGS += @GLIB_LIBS@
endif
libsensors_ssc_la_LIBADD = $(requiredlibs)
