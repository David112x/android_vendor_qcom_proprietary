ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/iqinterpolation
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                         \
    abf40interpolation.cpp                 \
    anr10interpolation.cpp                 \
    asf30interpolation.cpp                 \
    bc10interpolation.cpp                  \
    bls12interpolation.cpp                 \
    bpsgic30interpolation.cpp              \
    hdr22interpolation.cpp                 \
    hdr23interpolation.cpp                 \
    hdr30interpolation.cpp                 \
    linearization34interpolation.cpp       \
    bpspdpc20interpolation.cpp             \
    cac22interpolation.cpp                 \
    cac23interpolation.cpp                 \
    cc13interpolation.cpp                  \
    cv12interpolation.cpp                  \
    cvp10interpolation.cpp                 \
    demosaic36interpolation.cpp            \
    demosaic37interpolation.cpp            \
    dsx10interpolation.cpp                 \
    gamma15interpolation.cpp               \
    gamma16interpolation.cpp               \
    gra10interpolation.cpp                 \
    gtm10interpolation.cpp                 \
    hnr10interpolation.cpp                 \
    ica10interpolation.cpp                 \
    ica20interpolation.cpp                 \
    ica30interpolation.cpp                 \
    ifeabf34interpolation.cpp              \
    ifebpcbcc50interpolation.cpp           \
    ifecc12interpolation.cpp               \
    ifehdr20interpolation.cpp              \
    ifelinearization33interpolation.cpp    \
    ifepdpc11interpolation.cpp             \
    ipe2dlut10interpolation.cpp            \
    ipecs20interpolation.cpp               \
    lenr10interpolation.cpp                \
    lsc34interpolation.cpp                 \
    lsc40interpolation.cpp                 \
    ltm13interpolation.cpp                 \
    ltm14interpolation.cpp                 \
    pdpc30interpolation.cpp                \
    pedestal13interpolation.cpp            \
    sce11interpolation.cpp                 \
    tf10interpolation.cpp                  \
    tf20interpolation.cpp                  \
    tintless20interpolation.cpp            \
    tmc10interpolation.cpp                 \
    tmc11interpolation.cpp                 \
    tmc12interpolation.cpp                 \
    upscale20interpolation.cpp

LOCAL_INC_FILES :=                         \
    abf40interpolation.h                   \
    anr10interpolation.h                   \
    asf30interpolation.h                   \
    bc10interpolation.h                    \
    bls12interpolation.h                   \
    bpsgic30interpolation.h                \
    hdr22interpolation.h                   \
    hdr23interpolation.h                   \
    hdr30interpolation.h                   \
    linearization34interpolation.h         \
    bpspdpc20interpolation.h               \
    cac22interpolation.h                   \
    cac23interpolation.h                   \
    cc13interpolation.h                    \
    cv12interpolation.h                    \
    cvp10interpolation.h                   \
    demosaic36interpolation.h              \
    demosaic37interpolation.h              \
    dsx10interpolation.h                   \
    gamma15interpolation.h                 \
    gamma16interpolation.h                 \
    gra10interpolation.h                   \
    gtm10interpolation.h                   \
    hnr10interpolation.h                   \
    ica10interpolation.h                   \
    ica20interpolation.h                   \
    ica30interpolation.h                   \
    ifeabf34interpolation.h                \
    ifebpcbcc50interpolation.h             \
    ifecc12interpolation.h                 \
    ifehdr20interpolation.h                \
    ifelinearization33interpolation.h      \
    ifepdpc11interpolation.h               \
    ipecs20interpolation.h                 \
    ipe2dlut10interpolation.h              \
    iqcommondefs.h                         \
    lenr10interpolation.h                  \
    lsc34interpolation.h                   \
    lsc40interpolation.h                   \
    ltm13interpolation.h                   \
    ltm14interpolation.h                   \
    pdpc30interpolation.h                  \
    pedestal13interpolation.h              \
    sce11interpolation.h                   \
    tf10interpolation.h                    \
    tf20interpolation.h                    \
    tintless20interpolation.h              \
    tmc10interpolation.h                   \
    tmc11interpolation.h                   \
    tmc12interpolation.h                   \
    upscale20interpolation.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)              \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix   \
    $(CAMX_CHICDK_API_PATH)/generated/g_parser      \
    $(CAMX_PATH)/src/hwl/iqsetting

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxiqinterpolation

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
