# Makefile.am - Automake script for see_workhorse

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -Wno-unused-variable \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./ \
              -I../see_salt

requiredlibs = ../see_salt/libSEESalt.la -llog -lcutils

cpp_sources = ./see_workhorse.cpp

bin_PROGRAMS =  see_workhorse
see_workhorse_SOURCES = $(cpp_sources)
see_workhorse_CC = @CC@
see_workhorse_CPPFLAGS = $(AM_CPPFLAGS)
if USE_GLIB
 see_workhorse_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 see_workhorse_LDFLAGS = @GLIB_LIBS@
endif
see_workhorse_LDADD = $(requiredlibs)
