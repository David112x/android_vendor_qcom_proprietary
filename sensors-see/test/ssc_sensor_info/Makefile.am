# Makefile.am - Automake script for ssc_sensor_info

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -Wno-unused-variable \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./ \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc/proto \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-diag-log/inc

requiredlibs = -lsensorslog -lsnsproto -lssc -lsnsdiaglog -ldl -llog -lcutils \
               $(PROTOBUF_LIBS)

c_sources = ./ssc_sensor_info.cpp

bin_PROGRAMS =  ssc_sensor_info
ssc_sensor_info_SOURCES = $(c_sources)
ssc_sensor_info_CC = @CC@
ssc_sensor_info_CPPFLAGS = $(AM_CPPFLAGS)
if USE_GLIB
 ssc_sensor_info_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 ssc_sensor_info_LDFLAGS = @GLIB_LIBS@
endif
ssc_sensor_info_LDADD = $(requiredlibs)
