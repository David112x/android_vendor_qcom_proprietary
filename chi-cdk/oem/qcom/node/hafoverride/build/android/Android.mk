ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/node/hafoverride
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CHI-CDK project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_SRC_FILES :=  \
    src/camxhafoverride.cpp               \
    src/camxhafoverridetofdecorator.cpp   \
    src/camxhafoverridepdaf.cpp           \
    src/camxhafoverridepdaftofmixer.cpp

LOCAL_INC_FILES :=  \
    inc/camxhafoverride.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

CHROMATIX_VERSION := 0x0310
# Paths to included headers
LOCAL_C_INCLUDES :=                                     \
    $(CAMX_C_INCLUDES)                                  \
    $(CAMX_CHICDK_OEM_PATH)/qcom/node/hafoverride/inc

# Libraries to link
LOCAL_WHOLE_STATIC_LIBRARIES := \
    libcamxosutils              \
    libcamxutils

LOCAL_SHARED_LIBRARIES :=   \
    libcutils               \
    liblog                  \
    libofflinelog           \
    libsync

LOCAL_LDLIBS := -lz

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := com.qti.stats.hafoverride

LOCAL_MODULE_RELATIVE_PATH := $(CAMX_LIB_OUTPUT_PATH)

LOCAL_LDFLAGS :=
include $(BUILD_SHARED_LIBRARY)
#-include $(CAMX_CHECK_WHINER)
