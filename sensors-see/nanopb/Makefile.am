# Makefile.am - Automake script for nanopb

AM_CFLAGS = -Werror \
            -Wall \
            -Wextra \
            -Wno-missing-field-initializers \
            -Wno-maybe-uninitialized \
            -std=c11 \
            -DPB_FIELD_16BIT \
            -I./inc/

c_sources = ./src/pb_common.c \
            ./src/pb_decode.c \
            ./src/pb_encode.c

requiredlibs =

include_HEADERS = ./inc/pb_common.h \
                  ./inc/pb_decode.h \
                  ./inc/pb_encode.h \
                  ./inc/pb.h

library_includedir = $(pkgincludedir)

lib_LTLIBRARIES = libnanopb.la
libnanopb_la_CC = @CC@
libnanopb_la_SOURCES = $(cpp_sources)
libnanopb_la_CFLAGS = $(AM_CFLAGS)
libnanopb_la_LDFLAGS = -shared -version-info 1:0:0
libnanopb_la_LIBADD = $(requiredlibs)
