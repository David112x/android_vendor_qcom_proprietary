ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2swmf
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CHI-CDK project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_SRC_FILES :=  \
    chifeature2swmf.cpp

LOCAL_INC_FILES :=  \
    chifeature2swmf.h

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                      \
    $(CAMX_C_INCLUDES)                   \
    $(CAMX_CHICDK_PATH)/core/chifeature2

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_WHOLE_STATIC_LIBRARIES +=     \
    libchiutils                     \

LOCAL_SHARED_LIBRARIES +=           \
    com.qti.chi.override            \
    com.qti.feature2.gs             \
    libcamera_metadata              \
    libchilog                       \
    libcutils                       \
    libhardware                     \
    libhidlbase                     \
    liblog                          \
    libqdMetaData                   \
    libsync                         \
    libutils                        \
    vendor.qti.hardware.vpp@1.1     \
    vendor.qti.hardware.vpp@1.2

# Binary name
LOCAL_MODULE := com.qti.feature2.swmf

LOCAL_LDLIBS := -lz

include $(BUILD_SHARED_LIBRARY)

-include $(CAMX_CHECK_WHINER)
