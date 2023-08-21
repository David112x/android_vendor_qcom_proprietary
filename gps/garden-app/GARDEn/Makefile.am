AM_CFLAGS = \
    -DDEBUG \
    -I./locationclient_api \
    -I./clientapi_case \
    -I./garden_framework \
    $(GPSUTILS_CFLAGS) \
    $(LOCCLIENTAPI_CFLAGS) \
    -std=c++11

if USE_EXTERNAL_AP
AM_CFLAGS += -DFEATURE_EXTERNAL_AP
else
AM_CFLAGS += \
    -I./gnss_case \
    -I./geofence_case \
    -I./flp_case \
    -I./gnss_multiclient_case \
    -I./locationapi_api \
    $(LOCCORE_CFLAGS) \
    $(LOCDIAGIFACE_CFLAGS) \
    $(DIAG_CFLAGS)
endif

AM_CPPFLAGS = $(AM_CFLAGS)

## Required libraries
required_libs = \
    $(LOCCLIENTAPI_LIBS) \
    $(GPSUTILS_LIBS)

if !USE_EXTERNAL_AP
required_libs += $(LOCATIONAPI_LIBS) $(LOCCORE_LIBS)
endif

## libgarden header sources
libgarden_h_sources = \
    garden_framework/IGardenCase.h \
    garden_framework/GardenFramework.h \
    garden_framework/IGnssAPI.h \
    garden_framework/GnssCbBase.h \
    garden_framework/GardenUtil.h \
    locationclient_api/GnssLocationClientAPI.h

if !USE_EXTERNAL_AP
libgarden_h_sources += \
    gnss_multiclient_case/GnssMultiClientCaseUtils.h \
    locationapi_api/GnssLocationAPI.h \
    locationapi_api/GnssLocationAPIClient.h \
    locationapi_api/GeofenceLocationAPIClient.h \
    locationapi_api/GnssMultiClientLocationAPIClient.h \
    locationapi_api/FlpLocationAPIClient.h
endif

## libgarden cases header sources
libgarden_cases_h_sources = \
    clientapi_case/LocationClientApiCase.h

if !USE_EXTERNAL_AP
libgarden_cases_h_sources += \
    gnss_case/GnssCase.h \
    gnss_case/GnssCb.h \
    geofence_case/GeofenceCb.h \
    geofence_case/GeofenceCase.h \
    flp_case/FlpCb.h \
    flp_case/FlpCase.h \
    gnss_multiclient_case/GnssMultiClientCase.h \
    gnss_multiclient_case/GnssMultiClientCaseUtils.h \
    gnss_multiclient_case/GnssMultiClientCb.h \
    gnss_multiclient_case/GnssMultiClientFileTest.h
endif

## libgarden C sources
libgarden_c_sources = \
    garden_framework/GardenFramework.cpp \
    garden_framework/GardenUtil.cpp \
    garden_framework/GnssCbBase.cpp \
    locationclient_api/GnssLocationClientAPI.cpp

if !USE_EXTERNAL_AP
libgarden_c_sources += \
    locationapi_api/GnssLocationAPI.cpp \
    locationapi_api/GnssLocationAPIClient.cpp \
    locationapi_api/FlpLocationAPIClient.cpp \
    locationapi_api/GnssMultiClientLocationAPIClient.cpp \
    locationapi_api/GeofenceLocationAPIClient.cpp
endif

## libgarden cases C sources
libgarden_cases_c_sources = \
    clientapi_case/LocationClientApiCase.cpp

if !USE_EXTERNAL_AP
libgarden_cases_c_sources += \
    gnss_case/GnssCase.cpp \
    gnss_case/GnssCb.cpp \
    geofence_case/GeofenceCb.cpp \
    geofence_case/GeofenceCase.cpp \
    flp_case/FlpCase.cpp \
    flp_case/FlpCb.cpp \
    gnss_multiclient_case/GnssMultiClientCase.cpp \
    gnss_multiclient_case/GnssMultiClientCaseUtils.cpp \
    gnss_multiclient_case/GnssMultiClientCb.cpp \
    gnss_multiclient_case/GnssMultiClientFileTest.cpp
endif

## Automotive specific addition to various flags and variables
if USE_FEATURE_AUTOMOTIVE
    AM_CFLAGS += -I./cdfwapi_case \
                 $(CDFW_CFLAGS) \
                 $(CDFWREMOTEAPI_CFLAGS) \
                 -DFEATURE_AUTOMOTIVE
    AM_CPPFLAGS += -DFEATURE_AUTOMOTIVE
    required_libs += $(CDFW_LIBS)
    required_libs += $(CDFWREMOTEAPI_LIBS)
    libgarden_h_sources += cdfwapi_case/cdfwApiCase.h
    libgarden_cases_h_sources += cdfwapi_case/cdfwApiCase.h
    libgarden_c_sources += cdfwapi_case/cdfwApiCase.cpp
    libgarden_cases_c_sources += cdfwapi_case/cdfwApiCase.cpp
endif

library_includedir = $(pkgincludedir)
library_include_HEADERS = $(libgarden_h_sources)

######################
# Build libgarden
######################
libgarden_la_SOURCES = $(libgarden_c_sources)

if USE_GLIB
libgarden_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libgarden_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -avoid-version
libgarden_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libgarden_la_CFLAGS = $(AM_CFLAGS)
libgarden_la_LDFLAGS = -lpthread -shared -version-info 1:0:0
libgarden_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

library_include_HEADERS = $(libgarden_h_sources)
libgarden_la_LIBADD = -ldl $(LOC_LIBS) $(required_libs)
if !USE_EXTERNAL_AP
libgarden_la_LIBADD += -lcutils
endif

lib_LTLIBRARIES = libgarden.la

######################
# Build libgarden_haltests_e2e
######################
libgarden_haltests_e2e_la_SOURCES = $(libgarden_cases_c_sources)

if USE_GLIB
libgarden_haltests_e2e_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libgarden_haltests_e2e_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -avoid-version
libgarden_haltests_e2e_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libgarden_haltests_e2e_la_CFLAGS = $(AM_CFLAGS)
libgarden_haltests_e2e_la_LDFLAGS = -lpthread -shared -version-info 1:0:0
libgarden_haltests_e2e_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

library_include_HEADERS = $(libgarden_cases_h_sources)
libgarden_haltests_e2e_la_LIBADD = libgarden.la -ldl $(LOC_LIBS) $(required_libs)
if !USE_EXTERNAL_AP
libgarden_la_LIBADD += -lcutils
endif

lib_LTLIBRARIES += libgarden_haltests_e2e.la

######################
# Build sample_case
######################
libgardencasesample_la_SOURCES = sample_case/SampleCase.cpp

if USE_GLIB
libgardencasesample_la_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
libgardencasesample_la_LDFLAGS = -lstdc++ -lpthread @GLIB_LIBS@ -shared -avoid-version
libgardencasesample_la_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
libgardencasesample_la_CFLAGS = $(AM_CFLAGS)
libgardencasesample_la_LDFLAGS = -lpthread -shared -version-info 1:0:0
libgardencasesample_la_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

libgardencasesample_la_LIBADD = libgarden.la -ldl $(LOC_LIBS) $(required_libs)
if !USE_EXTERNAL_AP
libgarden_la_LIBADD += -lcutils
endif

lib_LTLIBRARIES += libgardencasesample.la

######################
# Build garden_app
######################
garden_app_SOURCES = main.cpp

if USE_GLIB
garden_app_CFLAGS = -DUSE_GLIB $(AM_CFLAGS) @GLIB_CFLAGS@
garden_app_LDFLAGS = -lstdc++ -g -Wl,-z,defs -lpthread @GLIB_LIBS@ -shared -version-info 1:0:0
garden_app_CPPFLAGS = -DUSE_GLIB $(AM_CFLAGS) $(AM_CPPFLAGS) @GLIB_CFLAGS@
else
garden_app_CFLAGS = $(AM_CFLAGS)
garden_app_LDFLAGS = -Wl,-z,defs -lpthread -shared -version-info 1:0:0
garden_app_CPPFLAGS = $(AM_CFLAGS) $(AM_CPPFLAGS)
endif

garden_app_LDADD = libgarden.la libgarden_haltests_e2e.la -ldl $(required_libs)
if !USE_EXTERNAL_AP
libgarden_la_LIBADD += -lcutils
endif

bin_PROGRAMS = garden_app
