ifeq ($(CAMX_CHICDK_CORE_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_CORE_PATH := $(abspath $(LOCAL_PATH)/..)
else
LOCAL_PATH := $(CAMX_CHICDK_CORE_PATH)/chiutils
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES :=                      \
    chxblmclient.h                      \
    chxdefs.h                           \
    chxincs.h                           \
    chxutils.h                          \
    chxperf.h                           \
    chxmetadata.h

LOCAL_SRC_FILES :=                                \
    chxblmclient.cpp                              \
    chxutils.cpp                                  \
    chxperf.cpp                                   \
    chxmetadata.cpp

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                         \
    $(CAMX_C_INCLUDES)

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_LDFLAGS :=
LOCAL_LDLIBS :=

LOCAL_SHARED_LIBRARIES +=           \
    libchilog

LOCAL_MODULE := libchiutils

include $(BUILD_STATIC_LIBRARY)

-include $(CAMX_CHECK_WHINER)
