# Makefile.am - Automake script for sensors-see-commonsys-intf

ACLOCAL_AMFLAGS = -I m4

dirs := sensors-log ssc/proto ssc sensors-diag-log USTA/native

SUBDIRS = $(dirs)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = sensors-see-commonsys-intf.pc

EXTRA_DIST = $(pkgconfig_DATA)
