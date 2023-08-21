# Makefile.am - Automake script for Sensordaemon

AM_CPPFLAGS = -Werror \
              -Wall \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./src/ \
              -I../sscrpcd/inc/ \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc/proto/ \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc

requiredlibs = -lsensorslog -lsnsproto -lssc -llog -lcutils $(DIAG_LIBS) $(PROTOBUF_LIBS)

cpp_sources = ./src/sensordaemon.cpp \
              ./src/sns_reg_config.cpp \
              ./src/aont.cpp

bin_PROGRAMS = sensors.qti
sensors_qti_SOURCES = $(cpp_sources)
sensors_qti_CC = @CC@
sensors_qti_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS) @DIAG_CFLAGS@ @PROTOBUF_CFLAGS@
if USE_GLIB
 sensors_qti_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 sensors_qti_LDFLAGS = @GLIB_LIBS@
endif
sensors_qti_LDADD = $(requiredlibs)
