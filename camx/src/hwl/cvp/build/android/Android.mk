ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/cvp
endif
include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=  \
    camxcvpnode.cpp

LOCAL_INC_FILES :=  \
    camxcvpnode.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                                   \
    $(CAMX_CHICDK_API_PATH)/common                                       \
    $(CAMX_PATH)/src/core                                                \
    $(CAMX_PATH)/src/generated/g_parser                                  \
    $(CAMX_PATH)/src/hwl/cvp                                             \
    $(CAMX_PATH)/src/hwl/titan17x                                        \
    $(CAMX_PATH)/src/hwl/titan17x/regmap/titan170                        \
    $(CAMX_PATH)/src/hwl/isphwsetting                                    \
    $(CAMX_PATH)/src/hwl/isphwsetting/pipeline                           \
    $(CAMX_PATH)/src/hwl/ispiqmodule                                     \
    $(CAMX_PATH)/src/hwl/iqsetting                                       \
    $(TARGET_OUT_HEADERS)/camx                                           \
    $(TARGET_OUT_HEADERS)/synx                                           \
    $(TARGET_OUT_HEADERS)/cvp/v2.0

ifeq ($(IQSETTING),OEM1)
LOCAL_C_INCLUDES += \
    $(CAMX_OEM1IQ_PATH)/iqsetting
else
LOCAL_C_INCLUDES += \
    $(CAMX_PATH)/src/hwl/iqinterpolation
endif

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxhwlcvp

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
