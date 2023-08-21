ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/iqsetting
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                         \
    abf40setting.cpp                       \
    anr10setting.cpp                       \
    asf30setting.cpp                       \
    bc10setting.cpp                        \
    bls12setting.cpp                       \
    bpsgic30setting.cpp                    \
    hdr22setting.cpp                       \
    hdr23setting.cpp                       \
    hdr30setting.cpp                       \
    linearization34setting.cpp             \
    bpspdpc20setting.cpp                   \
    cac22setting.cpp                       \
    cac23setting.cpp                       \
    cc13setting.cpp                        \
    cst12setting.cpp                       \
    cv12setting.cpp                        \
    cvp10setting.cpp                       \
    demosaic36setting.cpp                  \
    demosaic37setting.cpp                  \
    demux13setting.cpp                     \
    dsx10setting.cpp                       \
    gamma15setting.cpp                     \
    gamma16setting.cpp                     \
    gra10setting.cpp                       \
    gtm10setting.cpp                       \
    hnr10setting.cpp                       \
    icasetting.cpp                         \
    icautils.cpp                           \
    ifeabf34setting.cpp                    \
    ifebpcbcc50setting.cpp                 \
    ifecc12setting.cpp                     \
    ifehdr20setting.cpp                    \
    ifelinearization33setting.cpp          \
    ifepdpc11setting.cpp                   \
    ipecs20setting.cpp                     \
    ipe2dlut10setting.cpp                  \
    iqsettingutil.cpp                      \
    lenr10setting.cpp                      \
    lsc34setting.cpp                       \
    lsc40setting.cpp                       \
    ltm13setting.cpp                       \
    ltm14setting.cpp                       \
    pdpc30setting.cpp                      \
    pedestal13setting.cpp                  \
    sce11setting.cpp                       \
    tf10setting.cpp                        \
    tf20setting.cpp                        \
    upscale12setting.cpp                   \
    upscale20setting.cpp                   \
    upscale20setting_misc.cpp              \
    wb12setting.cpp                        \
    wb13setting.cpp


LOCAL_INC_FILES :=                         \
    abf40setting.h                         \
    anr10setting.h                         \
    asf30setting.h                         \
    bc10setting.h                          \
    bls12setting.h                         \
    bpsgic30setting.h                      \
    hdr22setting.h                         \
    hdr23setting.h                         \
    hdr30setting.h                         \
    linearization34setting.h               \
    bpspdpc20setting.h                     \
    cac22setting.h                         \
    cac23setting.h                         \
    camxiqfunctiontable.h                  \
    cc13setting.h                          \
    cst12setting.h                         \
    cv12setting.h                          \
    cvp10setting.h                         \
    demosaic36setting.h                    \
    demosaic37setting.h                    \
    demux13setting.h                       \
    dsx10setting.h                         \
    gamma15setting.h                       \
    gamma16setting.h                       \
    gra10setting.h                         \
    gtm10setting.h                         \
    hnr10setting.h                         \
    icasetting.h                           \
    icautils.h                             \
    ifeabf34setting.h                      \
    ifebpcbcc50setting.h                   \
    ifecc12setting.h                       \
    ifehdr20setting.h                      \
    ifelinearization33setting.h            \
    ifepdpc11setting.h                     \
    ipecs20setting.h                       \
    ipe2dlut10setting.h                    \
    iqsettingutil.h                        \
    lenr10setting.h                        \
    lsc34setting.h                         \
    lsc40setting.h                         \
    ltm13setting.h                         \
    ltm14setting.h                         \
    pdpc30setting.h                        \
    pedestal13setting.h                    \
    sce11setting.h                         \
    tf10setting.h                          \
    tf20setting.h                          \
    upscale12setting.h                     \
    upscale20setting.h                     \
    upscale20setting_misc.h                \
    wb12setting.h                          \
    wb13setting.h


# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)              \
    $(CAMX_CHICDK_API_PATH)/generated/g_parser      \
    $(TARGET_OUT_HEADERS)/camx                      \
    $(TARGET_OUT_HEADERS)/camx/titan17x             \


ifeq ($(IQSETTING),OEM1)
LOCAL_C_INCLUDES +=                        \
    $(CAMX_OEM1IQ_PATH)/iqsetting
else
LOCAL_C_INCLUDES +=                                 \
    $(CAMX_PATH)/src/hwl/iqinterpolation
endif

LOCAL_C_INCLUDES +=                                 \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix

LOCAL_STATIC_LIBRARIES :=   \
    libnc                   \
    libcamxgenerated

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxiqsetting

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
