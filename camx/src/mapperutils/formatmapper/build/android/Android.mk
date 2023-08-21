ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/mapperutils/formatmapper
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                  \
    camxdisplayconfig.cpp           \
    camximageformatmapper.cpp       \
    camximageformatutils.cpp        \

LOCAL_INC_FILES :=                  \
    camxdisplayconfig.h             \
    camxformats.h                   \
    camximageformatmapper.h         \
    camximageformatutils.h          \

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                         \
                    $(CAMX_PATH)/src/mapperutils/extformatutil \

# Service manager header file path
LOCAL_C_INCLUDES += $(SOONG_OUT_DIR)/.intermediates/system/libhidl/transport/manager/1.1/android.hidl.manager@1.1_genc++_headers/gen/android/hidl/manager/1.1

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Libraries to link
LOCAL_WHOLE_STATIC_LIBRARIES := \
    libcamxosutils              \
    libcamxutils                \

LOCAL_SHARED_LIBRARIES +=        \
    libutils                     \
    libhidlbase                  \

DISPLAY_SOONG_DIR := $(SOONG_OUT_DIR)/.intermediates/vendor/qcom/opensource/interfaces/display

# Display config header file path
LOCAL_C_INCLUDES += $(DISPLAY_SOONG_DIR)/config/1.9/vendor.display.config@1.9_genc++_headers/gen/vendor/display/config/1.9

LOCAL_SHARED_LIBRARIES +=                   \
    vendor.display.config@1.0               \
    vendor.display.config@1.1               \
    vendor.display.config@1.2               \
    vendor.display.config@1.3               \
    vendor.display.config@1.4               \
    vendor.display.config@1.5               \
    vendor.display.config@1.6               \
    vendor.display.config@1.7               \
    vendor.display.config@1.8               \
    vendor.display.config@1.9

# Format mapper dependencies
LOCAL_C_INCLUDES += $(DISPLAY_SOONG_DIR)/mapper/

LOCAL_SHARED_LIBRARIES +=                   \
    android.hardware.graphics.mapper@2.0    \
    android.hardware.graphics.mapper@2.1    \
    android.hardware.graphics.mapper@3.0    \
    vendor.qti.hardware.display.mapper@1.0  \
    vendor.qti.hardware.display.mapper@2.0  \
    vendor.qti.hardware.display.mapper@3.0  \

# Binary name
LOCAL_MODULE := libcamximageformatutils

LOCAL_LDLIBS := -lz

include $(CAMX_BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
LOCAL_COPY_HEADERS_TO := camx
LOCAL_COPY_HEADERS    := camximageformatutils.h          \
                         camxformats.h                   \

include $(BUILD_COPY_HEADERS)
