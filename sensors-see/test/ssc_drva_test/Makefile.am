# Makefile.am - Automake script for ssc_drva_test

AM_CPPFLAGS = -Werror \
              -Wall \
              -Wno-unused-parameter \
              -Wno-unused-variable \
              -fexceptions \
              -DSNS_LE_QCS605 \
              -I./ \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc/proto \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log \
              -I${WORKSPACE}/vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-diag-log/inc

requiredlibs = -lsensorslog -lsnsproto -lssc -lsnsdiaglog -ldl -llog -lcutils \
               $(PROTOBUF_LIBS)

c_sources = ./ssc_drva_test.cpp

bin_PROGRAMS =  ssc_drva_test
ssc_drva_test_SOURCES = $(c_sources)
ssc_drva_test_CC = @CC@
ssc_drva_test_CPPFLAGS = $(AM_CPPFLAGS)
if USE_GLIB
 ssc_drva_test_CPPFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 ssc_drva_test_LDFLAGS = @GLIB_LIBS@
endif
ssc_drva_test_LDADD = $(requiredlibs)
