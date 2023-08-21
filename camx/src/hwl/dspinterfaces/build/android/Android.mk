ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/dspinterfaces
endif

ifeq ($(TARGET_BOARD_PLATFORM),kona)
#binary in $(TARGET_OUT_VENDOR)/lib/dsp/cdsp
#no need load Prebuilt from APPS
else
ifeq ($(TARGET_BOARD_PLATFORM),lito)
#binary in $(TARGET_OUT_VENDOR)/lib/dsp/cdsp
#no need load Prebuilt from APPS
else
ifeq ($(TARGET_BOARD_PLATFORM),bengal)
#binary in $(TARGET_OUT_VENDOR)/lib/dsp/cdsp
#no need load Prebuilt from APPS
else
include $(CLEAR_VARS)
LOCAL_MODULE                := libdsp_streamer_skel
LOCAL_MODULE_SUFFIX         := .so
LOCAL_MODULE_CLASS          := ETC
ifeq ($(TARGET_BOARD_PLATFORM),msmnile)
LOCAL_SRC_FILES             := sm8150/libdsp_streamer_skel.so
else
ifeq ($(TARGET_BOARD_PLATFORM),sm6150)
LOCAL_SRC_FILES             := sm7150/libdsp_streamer_skel.so
else
ifeq ($(TARGET_BOARD_PLATFORM),sdm710)
LOCAL_SRC_FILES             := sdm710/libdsp_streamer_skel.so
else
ifeq ($(TARGET_BOARD_PLATFORM),lito)
LOCAL_SRC_FILES             := sm7150/libdsp_streamer_skel.so
else
endif
endif
endif
endif

LOCAL_MODULE_TAGS           := optional
LOCAL_MODULE_OWNER          := qti
LOCAL_PROPRIETARY_MODULE    := true
LOCAL_MODULE_PATH           := $(TARGET_OUT_VENDOR)/lib/rfsa/adsp
include $(BUILD_PREBUILT)

endif
endif
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_INC_FILES :=              \
    camxifedspinterface.h       \
    AEEStdDef.h                 \
    AEEStdErr.h                 \
    dsp_streamer.h              \
    dsp_streamer_callback.h     \
    remote64.h                  \
    remote.h                    \

LOCAL_SRC_FILES :=                       \
    camxifedspinterface.cpp              \
    dsp_streamer_callback_skel.c         \
    dsp_streamer_stub.c


# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)          \
    $(CAMX_PATH)/src/hwl/dspinterfaces

LOCAL_C_INCLUDES +=                                                 \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include              \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/camera/include/uapi

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)  \
    -Wno-uninitialized          \
    -Wno-unused-parameter       \
    -Wno-unused-variable
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES += libadsprpc

LOCAL_ADDITIONAL_DEPENDENCIES  := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

# Binary name
LOCAL_MODULE := libcamxdspstreamer

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
