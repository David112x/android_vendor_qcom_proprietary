# Makefile.am - Automake script for see_stability

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -Wno-unused-variable \
              -fexceptions \
              -std=c++11 \
              -DSNS_LE_QCS605 \
              -I./ \
              -I../see_salt

requiredlibs = ../see_salt/libSEESalt.la -llog -lcutils

cpp_sources = ./main.cpp \
              ./Parser.cpp \
              ./StabilityEngine.cpp \
              ./TestCase.cpp \
              ./Utility.cpp

bin_PROGRAMS =  see_stability
see_stability_SOURCES = $(cpp_sources)
see_stability_CC = @CC@
see_stability_CPPFLAGS = $(AM_CPPFLAGS)
if USE_GLIB
 see_stability_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 see_stability_LDFLAGS = @GLIB_LIBS@
endif
see_stability_LDADD = $(requiredlibs)
