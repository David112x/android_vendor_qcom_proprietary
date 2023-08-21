ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/titan17x
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                  \
    camxtitan17xcontext.cpp         \
    camxtitan17xfactory.cpp         \
    camxtitan17xhwl.cpp             \
    camxtitan17xsettingsmanager.cpp \
    camxtitan17xstatsparser.cpp     \
    g_camxtitan17xsettings.cpp

LOCAL_INC_FILES :=                  \
    camxtitan17xcontext.h           \
    camxtitan17xdefs.h              \
    camxtitan17xfactory.h           \
    camxtitan17xsettingsmanager.h   \
    camxtitan17xstatsparser.h       \
    g_camxtitan17xsettings.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                                       \
    $(CAMX_CHICDK_API_PATH)/chromatix/isp                                    \
    $(CAMX_CHICDK_API_PATH)/generated/g_facedetection                        \
    $(CAMX_CHICDK_API_PATH)/pdlib                                            \
    $(CAMX_CHICDK_API_PATH)/stats                                            \
    $(CAMX_PATH)/src/hwl/bps                                                 \
    $(CAMX_PATH)/src/hwl/cvp                                                 \
    $(CAMX_PATH)/src/hwl/fd                                                  \
    $(CAMX_PATH)/src/hwl/ife                                                 \
    $(CAMX_PATH)/src/hwl/ipe                                                 \
    $(CAMX_PATH)/src/hwl/iqsetting                                           \
    $(CAMX_PATH)/src/hwl/ispiqmodule                                         \
    $(CAMX_PATH)/src/hwl/isphwsetting                                        \
    $(CAMX_PATH)/src/hwl/isphwsetting/pipeline                               \
    $(CAMX_PATH)/src/hwl/jpeg                                                \
    $(CAMX_PATH)/src/hwl/lrme                                                \
    $(CAMX_PATH)/src/swl/fd/fdmanager                                        \
    $(CAMX_PATH)/src/swl/jpeg                                                \
    $(CAMX_PATH)/src/swl/offlinestats                                        \
    $(CAMX_PATH)/src/swl/ransac                                              \
    $(CAMX_PATH)/src/swl/sensor                                              \
    $(CAMX_PATH)/src/swl/stats                                               \
    $(CAMX_PATH)/src/core                                                    \
    $(CAMX_PATH)/src/core/ncs                                                \
    $(CAMX_PATH)/src/core/chi                                                \
    $(CAMX_OUT_HEADERS)                                                      \
    $(CAMX_OUT_HEADERS)/fd/fdengine                                          \
    $(CAMX_OUT_HEADERS)/swprocessalgo                                        \
    $(CAMX_OUT_HEADERS)/localhistogramalgo                                   \
    $(TARGET_OUT_HEADERS)/cvp/v2.0                                           \
    $(TARGET_OUT_HEADERS)/synx                                               \

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

ifeq ($(TARGET_BOARD_PLATFORM),kona)
LOCAL_CFLAGS += -DCVPENABLED
endif

# Binary name
LOCAL_MODULE := libcamxhwltitan17x

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
