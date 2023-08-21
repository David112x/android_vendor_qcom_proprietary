ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/swl/eisv3
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_INC_FILES :=              \
    camxchinodeeisv3.h

LOCAL_SRC_FILES :=              \
    camxchinodeeisv3.cpp

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)     \
    system/media/camera/include            \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix \
    $(CAMX_CHICDK_API_PATH)/generated/g_parser    \
    $(CAMX_CHICDK_API_PATH)/node                  \
    $(CAMX_PATH)/ext                       \
    $(CAMX_PATH)/src/hwl/ispiqmodule       \
    $(CAMX_PATH)/system/isalgo/common      \
    $(CAMX_PATH)/system/isalgo/eisv3algo

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

CHROMATIX_VERSION := 0x0310

LOCAL_SHARED_LIBRARIES +=      \
    libcamera_metadata         \
    libchilog                  \
    libcom.qti.chinodeutils

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libcamxgenerated            \
    libcamxosutils              \
    libcamxutils

LOCAL_LDLIBS := -lz

LOCAL_MODULE := com.qti.node.eisv3

LOCAL_MODULE_RELATIVE_PATH := $(CAMX_LIB_OUTPUT_PATH)

include $(BUILD_SHARED_LIBRARY)
