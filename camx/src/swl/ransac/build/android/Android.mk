ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/swl/ransac
endif
include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=  \
    camxransacnode.cpp

LOCAL_INC_FILES :=  \
    camxransacnode.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                                   \
    $(CAMX_CHICDK_API_PATH)/common                                       \
    $(CAMX_PATH)/src/core                                                \
    $(CAMX_PATH)/src/generated/g_parser                                  \
    $(CAMX_PATH)/src/swl/ransac                                          \
    $(CAMX_PATH)/src/hwl/titan17x                                        \
    $(CAMX_OUT_HEADERS)                                                  \

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_WHOLE_STATIC_LIBRARIES := libnc

# Binary name
LOCAL_MODULE := libcamxswlransac

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
