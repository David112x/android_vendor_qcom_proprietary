ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/osutils
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                   \
    camxmem.cpp                      \
    camxosutils$(CAMX_OS).cpp

LOCAL_INC_FILES :=     \
    camxmem.h          \
    camxosutils.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)    \
    hardware/qcom/display/include         \
    hardware/qcom/display/libqdutils

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES +=   \
    libcamximageformatutils

# Binary name
LOCAL_MODULE := libcamxosutils

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
LOCAL_COPY_HEADERS_TO := camx
LOCAL_COPY_HEADERS :=    camxmem.h          \
                         camxosutils.h

include $(BUILD_COPY_HEADERS)
