ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/node/nodeutils
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES := $(CAMX_CHICDK_API_PATH)/node/camxchinodeutil.h

LOCAL_SRC_FILES :=              \
    camxchinodeutil.cpp

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES :=                                                \
    $(CAMX_C_INCLUDES)                                             \
    $(CAMX_CHICDK_API_PATH)/utils                                  \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include             \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/camera/include/uapi

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES :=           \
    libcamera_metadata              \
    libchilog

LOCAL_MODULE := libcom.qti.chinodeutils

include $(BUILD_SHARED_LIBRARY)
