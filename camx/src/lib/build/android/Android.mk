ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/lib
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=

LOCAL_INC_FILES :=

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES :=

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Libraries to link
LOCAL_STATIC_LIBRARIES :=   \
    libcamxchi              \
    libcamxcore             \
    libcamxcsl              \
    libcamxofflinestats     \
    libcamxsettings         \
    libnc                   \


LOCAL_WHOLE_STATIC_LIBRARIES := \
    libcamxdspstreamer          \
    libcamxhwlbps               \
    libcamxgenerated            \
    libcamxhal                  \
    libcamxhalutils             \
    libcamxhwlfd                \
    libcamxhwlife               \
    libcamxhwlipe               \
    libcamxhwliqmodule          \
    libcamxswlfdmanager         \
    libcamxswljpeg              \
    libcamxhwljpeg              \
    libcamxhwllrme              \
    libcamxisphwsetting         \
    libcamxswlransac            \
    libcamxhwltitan17x          \
    libcamxiqsetting            \
    libcamxosutils              \
    libcamxstats                \
    libcamxsensor               \
    libcamxutils                \

ifeq ($(TARGET_BOARD_PLATFORM),kona)
LOCAL_WHOLE_STATIC_LIBRARIES += \
    libcamxhwlcvp
endif # kona

ifeq ($(IQSETTING),OEM1)
LOCAL_WHOLE_STATIC_LIBRARIES += \
    liboem1iqsetting
else
LOCAL_WHOLE_STATIC_LIBRARIES += \
    libcamxiqinterpolation
endif # ($(IQSETTING),OEM1)

LOCAL_SHARED_LIBRARIES +=               \
    libqdMetaData                       \
    libcamera_metadata                  \
    libcamxfdengine                     \
    libcamximageformatutils             \
    libcamxncs                          \
    libcamxstatscore

ifeq ($(TARGET_BOARD_PLATFORM),kona)
LOCAL_SHARED_LIBRARIES +=               \
    libcvp2                             \
    libcvp_common                       \
    libsynx
endif # kona

LOCAL_LDLIBS := -llog -lz -ldl

# Binary name
LOCAL_MODULE := $(CAMX_LIB)

# Deployment path under lib/lib64
LOCAL_MODULE_RELATIVE_PATH := hw

include $(CAMX_BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)
