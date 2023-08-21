# Makefile.am - Automake script for sensors-see

ACLOCAL_AMFLAGS = -I m4

dirs := sensors-hal nanopb sensordaemon test/hal_tests \
        test/ssc_drva_test test/ssc_sensor_info test/see_salt \
        test/see_workhorse test/see_resampler test/see_stability

SUBDIRS = $(dirs)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sensors-see.pc

EXTRA_DIST = $(pkgconfig_DATA)
