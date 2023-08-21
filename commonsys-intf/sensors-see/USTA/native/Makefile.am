# Makefile.am - Automake script for USTANative

AM_CPPFLAGS = -Werror \
              -Wno-unused-parameter \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./ \
              -I../../ssc/proto/ \
              -I../../ssc/ \
              -I../../sensors-diag-log/inc/

cpp_sources = ./sensor_cxt.cpp \
              ./sensor_suid.cpp \
              ./sensor.cpp \
              ./sensor_descriptor_pool.cpp \
              ./sensor_message_parser.cpp \
              ./sensor_attributes.cpp \
              ./logging_util.cpp

requiredlibs = ../../ssc/proto/libsnsproto.la ../../ssc/libssc.la ../../sensors-diag-log/libsnsdiaglog.la -llog -lutils $(PROTOBUF_LIBS)

library_includedir = $(pkgincludedir)

lib_LTLIBRARIES = libUSTANative.la
libUSTANative_la_CC = @CC@
libUSTANative_la_SOURCES = $(cpp_sources)
libUSTANative_la_CPPFLAGS = $(AM_CPPFLAGS) @PROTOBUF_CFLAGS@
libUSTANative_la_LDFLAGS = -shared -version-info 1:0:0
libUSTANative_la_LIBADD = $(requiredlibs)
