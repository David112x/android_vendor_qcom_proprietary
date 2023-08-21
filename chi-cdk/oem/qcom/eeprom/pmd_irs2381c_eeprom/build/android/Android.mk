ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/eeprom/pmd_irs2381c_eeprom
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_SRC_FILES :=                   \
    irs2381c_polar.cpp               \
    depthsensorutil.cpp

LOCAL_INC_FILES :=                   \
    eepromwrapper.h                  \
    depthsensorutil.h


# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := ..\..\sensor\irs2381c\libeepromcutter

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)

LOCAL_C_INCLUDES +=                 \
    $(CAMX_CHICDK_API_PATH)/sensor

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Library name
LOCAL_MODULE := com.qti.eeprom.irs2381c_polar

# Deployment path
LOCAL_MODULE_RELATIVE_PATH := camera

include $(BUILD_SHARED_LIBRARY)
