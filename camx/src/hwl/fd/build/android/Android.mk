ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/fd
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=      \
    camxfdhwnode.cpp    \
    camxfdhwutils.cpp

LOCAL_INC_FILES :=    \
    camxfdhwnode.h    \
    camxfdhwutils.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                                       \
    $(CAMX_PATH)/src/hwl/fd                                                  \
    $(CAMX_PATH)/src/hwl/titan17x                                            \
    $(CAMX_PATH)/src/swl/fd/fdmanager                                        \
    $(CAMX_CHICDK_API_PATH)/generated/g_facedetection                        \
    $(CAMX_OUT_HEADERS)                                                      \
    $(CAMX_OUT_HEADERS)/titan17x                                             \
    $(CAMX_OUT_HEADERS)/titan48x

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxhwlfd

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
