# Makefile.am - Automake script for see_resampler

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -Wno-unused-variable \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./ \
              -I../see_salt

requiredlibs = ../see_salt/libSEESalt.la -llog -lcutils

cpp_sources = ./see_resampler.cpp

bin_PROGRAMS =  see_resampler
see_resampler_SOURCES = $(cpp_sources)
see_resampler_CC = @CC@
see_resampler_CPPFLAGS = $(AM_CPPFLAGS)
if USE_GLIB
 see_resampler_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 see_resampler_LDFLAGS = @GLIB_LIBS@
endif
see_resampler_LDADD = $(requiredlibs)
