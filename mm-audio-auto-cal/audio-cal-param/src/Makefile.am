AM_CFLAGS = -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -g -O0 \
        -fno-inline \
        -fno-short-enums \
        -fpic \
        -I../inc \
        -I.

AM_CPPFLAGS = -I../inc \
              -I.

h_sources = ../inc/audcalparam_api.h \
            audcalparam_api_priv.h \
            audcalparam_serv_con.h 

c_sources = audcalparam_serv_con.c \
            audcalparam_api.c \
            ../test/testapi.c
  

library_includedir = $(pkgincludedir)
library_include_HEADERS = $(h_sources)

lib_LTLIBRARIES = libaudcalparam.la
libaudcalparam_la_CC = @CC@
libaudcalparam_la_SOURCES = $(c_sources) $(h_sources)
libaudcalparam_la_CFLAGS =  -DUSE_LIBACDBLOADER $(ACDBLOADER_CFLAGS) $(AM_CPPFLAGS) $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libaudcalparam_la_CPPFLAGS = $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libaudcalparam_la_LDFLAGS = -lm @GLIB_LIBS@ -shared
libaudcalparam_la_LIBADD = -ljson-c -ldl

bin_PROGRAMS = testapi
testapi_CC = @CC@
testapi_SOURCES = $(libaudcalparam_la_SOURCES)
testapi_CFLAGS= $(libaudcalparam_la_CFLAGS) -DAUDCALPARAM_DBG
testapi_CPPFLAGS= $(libaudcalparam_la_CPPFLAGS)
testapi_LDADD = $(libaudcalparam_la_LIBADD)
testapi = $(libaudcalparam_la_CFLAGS)
