# Makefile.am - Automake script for sensorslog

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -fexceptions \
              -I./

cpp_sources = sensors_log.cpp

requiredlibs = -llog -lutils

library_includedir = $(pkgincludedir)

include_HEADERS = ./sensors_log.h \
                  ./sensors_timeutil.h

lib_LTLIBRARIES = libsensorslog.la
libsensorslog_la_CC = @CC@
libsensorslog_la_SOURCES = $(cpp_sources)
libsensorslog_la_CPPFLAGS = $(AM_CPPFLAGS)
libsensorslog_la_LDFLAGS = -shared -version-info 1:0:0
libsensorslog_la_LIBADD = $(requiredlibs)
