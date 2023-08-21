ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2hdr
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CHI-CDK project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_SRC_FILES :=  \
    chifeature2hdr.cpp

LOCAL_INC_FILES :=  \
    chifeature2hdr.h

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                                              \
    $(CAMX_CHICDK_CORE_PATH)/lib/bitra                           \
    $(CAMX_C_INCLUDES)

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)


LOCAL_WHOLE_STATIC_LIBRARIES +=     \
    libchiutils.bitra               \

LOCAL_SHARED_LIBRARIES +=           \
    com.qti.chi.override.bitra      \
    com.qti.feature2.gs.bitra       \
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
LOCAL_MODULE := com.qti.feature2.hdr.bitra

LOCAL_LDLIBS := -lz

include $(BUILD_SHARED_LIBRARY)

-include $(CAMX_CHECK_WHINER)
