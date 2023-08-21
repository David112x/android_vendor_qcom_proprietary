ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/node/customhwnode
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES :=              \
    camxchinodecustomhwnode.h

LOCAL_SRC_FILES :=              \
    camxchinodecustomhwnode.cpp

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES :=                                                \
    $(CAMX_C_INCLUDES)                                             \
    $(CAMX_CHICDK_PATH)/api/node/                                  \
    system/media/camera/include                                    \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include             \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/camera/include/uapi

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES :=           \
    libcamera_metadata              \
    libcom.qti.chinodeutils         \
    libchilog                       \
    libcutils

LOCAL_MODULE := com.qti.node.customhwnode

LOCAL_MODULE_RELATIVE_PATH := $(CAMX_LIB_OUTPUT_PATH)

include $(BUILD_SHARED_LIBRARY)
