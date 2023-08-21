ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/swl/sensor
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=              \
    camxactuator.cpp            \
    camxexternalsensor.cpp      \
    camxois.cpp                 \
    camxcsiphysubmodule.cpp     \
    camxflash.cpp               \
    camximagesensorutils.cpp    \
    camxpdlibraryhandler.cpp    \
    camxsensornode.cpp          \
    camxsensorpickmode.cpp      \
    camxsensorsubmodulebase.cpp \
    camxtorchnode.cpp

LOCAL_INC_FILES :=              \
    camxactuator.h              \
    camxexternalsensor.h        \
    camxois.h                   \
    camxcsiphysubmodule.h       \
    camxflash.h                 \
    camximagesensorutils.h      \
    camxpdlibraryhandler.h      \
    camxsensornode.h            \
    camxsensorpickmode.h        \
    camxsensorsubmodulebase.h   \
    camxtorchnode.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)
LOCAL_C_INCLUDES += $(CAMX_CHICDK_API_PATH)/common
LOCAL_C_INCLUDES += $(CAMX_CHICDK_API_PATH)/fd
LOCAL_C_INCLUDES += $(CAMX_CHICDK_API_PATH)/pdlib
LOCAL_C_INCLUDES += $(CAMX_CHICDK_API_PATH)/pdlibwrapper

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Library name
LOCAL_MODULE := libcamxsensor

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
