# Makefile.am - Automake script for Sensors-diag log

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./inc/ \
              -I../sensors-log/ \
              -I../ssc/proto/ \
              -DGOOGLE_PROTOBUF_NO_RTTI

cpp_sources = ./src/sensor_diag_log.cpp

requiredlibs = ../sensors-log/libsensorslog.la ../ssc/libssc.la -llog -lcutils -lutils $(DIAG_LIBS)

library_includedir = $(pkgincludedir)

include_HEADERS = ./inc/sensor_diag_log.h

lib_LTLIBRARIES = libsnsdiaglog.la
libsnsdiaglog_la_CC = @CC@
libsnsdiaglog_la_SOURCES = $(cpp_sources)
libsnsdiaglog_la_CPPFLAGS = $(AM_CPPFLAGS) @DIAG_CFLAGS@
libsnsdiaglog_la_LDFLAGS = -shared -version-info 1:0:0
libsnsdiaglog_la_LIBADD = $(requiredlibs)
