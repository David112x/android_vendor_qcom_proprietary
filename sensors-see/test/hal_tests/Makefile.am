# Makefile.am - Automake script for hal_tests

AM_CFLAGS = -D_GNU_SOURCE \
            -DSNS_LE_QCS605 \
            -I../../../hardware/libhardware/include/sensors \
            -I../../sensors-hal/sensors/ \
            -I../../sensors-hal/framework/

requiredlibs = ../../sensors-hal/libsensors.ssc.la -llog -lcutils $(DIAG_LIBS) $(QMIFRAMEWORK_LIBS) $(LIBHARDWARE_LIBS)

c_sources = ./sns_espplus_testapp.c

bin_PROGRAMS =  sns_hal_batch
sns_hal_batch_SOURCES = $(c_sources)
sns_hal_batch_CC = @CC@
sns_hal_batch_CFLAGS = $(AM_CFLAGS)
if USE_GLIB
 sns_hal_batch_CFLAGS += -DUSE_GLIB @GLIB_CFLAGS@
 sns_hal_batch_LDFLAGS = @GLIB_LIBS@
endif
sns_hal_batch_LDADD = $(requiredlibs)
