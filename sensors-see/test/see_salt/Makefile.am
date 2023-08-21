# Makefile.am - Automake script for SEESalt libraries

AM_CPPFLAGS = -Werror \
              -Wno-unused-parameter \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./ \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/USTA/native

cpp_sources = ./salt_usta.cpp \
              ./see_salt.cpp \
              ./por.cpp \
              ./parse.cpp \
              ./see_salt_payloads.cpp \
              ./see_salt_sensor.cpp

requiredlibs = -lUSTANative -llog -lutils

library_includedir = $(pkgincludedir)

lib_LTLIBRARIES = libSEESalt.la
libSEESalt_la_CC = @CC@
libSEESalt_la_SOURCES = $(cpp_sources)
libSEESalt_la_CPPFLAGS = $(AM_CPPFLAGS) @PROTOBUF_CFLAGS@
libSEESalt_la_LDFLAGS = -shared -version-info 1:0:0
libSEESalt_la_LIBADD = $(requiredlibs)

