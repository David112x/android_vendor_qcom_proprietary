# Makefile.am - Automake script for ssc

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./ \
              -I./proto/ \
              -I../sensors-log/

cpp_sources = ./wakelock_utils.cpp \
              ./ssc_connection.cpp \
              ./ssc_utils.cpp \
              ./sns_client_api_v01.c

requiredlibs = ./proto/libsnsproto.la ../sensors-log/libsensorslog.la $(DIAG_LIBS) $(QMIFRAMEWORK_LIBS) $(PROTOBUF_LIBS)

library_includedir = $(pkgincludedir)

include_HEADERS = ./sns_client_api_v01.h \
                  ./ssc_connection.h \
                  ./ssc_utils.h \
                  ./wakelock_utils.h \
                  ./worker.h

lib_LTLIBRARIES = libssc.la
libssc_la_CC = @CC@
libssc_la_SOURCES = $(cpp_sources)
libssc_la_CPPFLAGS = $(AM_CPPFLAGS) @DIAG_CFLAGS@ @QMIFRAMEWORK_CFLAGS@ @PROTOBUF_CFLAGS@
libssc_la_LDFLAGS = -shared -version-info 1:0:0 @LDFLAGS@
if USE_GLIB
 libssc_la_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 libssc_la_LDFLAGS += @GLIB_LIBS@
endif
libssc_la_LIBADD = $(requiredlibs)
