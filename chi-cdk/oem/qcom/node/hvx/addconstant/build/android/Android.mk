ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/node/hvx/addconstant
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
LOCAL_MODULE                := libdsp_streamer_add_constant
LOCAL_MODULE_SUFFIX         := .so
LOCAL_MODULE_CLASS          := ETC
ifeq ($(TARGET_BOARD_PLATFORM),msmnile)
LOCAL_SRC_FILES             := sm8150/libdsp_streamer_add_constant.so
else
ifeq ($(TARGET_BOARD_PLATFORM),sm6150)
LOCAL_SRC_FILES             := sm7150/libdsp_streamer_add_constant.so
else
ifeq ($(TARGET_BOARD_PLATFORM),sdm710)
LOCAL_SRC_FILES             := sdm710/libdsp_streamer_add_constant.so
else
ifeq ($(TARGET_BOARD_PLATFORM),lito)
LOCAL_SRC_FILES             := sm7150/libdsp_streamer_add_constant.so
else
LOCAL_SRC_FILES             := libdsp_streamer_add_constant.so
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
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES :=

LOCAL_SRC_FILES :=              \
    camxchihvxaddconstant.cpp

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES +=             \
    $(CAMX_C_INCLUDES)

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary Name
LOCAL_MODULE := com.qti.hvx.addconstant

LOCAL_SHARED_LIBRARIES :=           \
    libchilog

LOCAL_MODULE_RELATIVE_PATH:= $(CAMX_LIB_OUTPUT_PATH)
LOCAL_HEADER_LIBRARIES := libutils_headers

include $(BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)
