ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/test/chiofflinepostproctest
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CFLAGS += -fexceptions            \
                -g                      \
                -Wno-unused-variable

LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_C_INCLUDES +=                 \
    $(CAMX_C_INCLUDES)              \
    $(LOCAL_PATH)/                  \
    system/libhidl/base/include     \

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_INC_FILES :=                  \
    chiofflinepostproccallbacks.h   \

LOCAL_SRC_FILES :=                          \
    chiofflinepostproccallbacks.cpp         \
    chiofflinepostproctest.cpp              \

LOCAL_SHARED_LIBRARIES +=                   \
    android.hardware.graphics.allocator@3.0 \
    android.hardware.graphics.common@1.1    \
    android.hardware.graphics.mapper@3.0    \
    libcamera_metadata                      \
    libcutils                               \
    libhidlbase                             \
    liblog                                  \
    libsync                                 \
    libutils                                \
    vendor.qti.hardware.camera.postproc@1.0 \

LOCAL_LDLIBS :=                 \
    -llog                       \
    -lz                         \
    -ldl

LOCAL_LDFLAGS :=

# Deployment path under bin
LOCAL_MODULE_RELATIVE_PATH := ../bin
# Binary name
LOCAL_MODULE := chiofflinepostproctest
LOCAL_PROPRIETARY_MODULE  := true

include $(BUILD_EXECUTABLE)
