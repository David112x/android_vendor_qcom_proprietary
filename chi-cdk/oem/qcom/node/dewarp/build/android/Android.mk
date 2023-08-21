ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/node/dewarp/
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES :=              \
    camxchinodedewarp.h

LOCAL_SRC_FILES :=              \
    camxchinodedewarp.cpp

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                                         \
    $(CAMX_C_INCLUDES)                                      \
    frameworks/native/libs/arect/include                    \
    frameworks/native/libs/nativewindow/include             \
    frameworks/native/libs/ui/include                       \
    frameworks/native/libs/nativebase/include               \
    $(TARGET_OUT_HEADERS)/adreno                            \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr              \
    hardware/qcom/display/gralloc                           \
    hardware/qcom/display/libqdutils                        \
    hardware/qcom/display/include                           \
    device/generic/goldfish-opengl/shared/OpenglCodecCommon \
    device/generic/goldfish-opengl/system/GLESv2_enc        \
    system/media/camera/include

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libcamxosutils              \
    libcamxutils

LOCAL_SHARED_LIBRARIES :=   \
    libcamera_metadata      \
    libchilog               \
    libcom.qti.chinodeutils \
    libcutils               \
    libutils                \
    liblog                  \
    libofflinelog           \
    libhardware             \
    libqdMetaData           \
    libsync

LOCAL_LDLIBS := -lz

LOCAL_MODULE := com.qti.node.dewarp

LOCAL_MODULE_RELATIVE_PATH := $(CAMX_LIB_OUTPUT_PATH)

include $(BUILD_SHARED_LIBRARY)
#-include $(CAMX_CHECK_WHINER)
