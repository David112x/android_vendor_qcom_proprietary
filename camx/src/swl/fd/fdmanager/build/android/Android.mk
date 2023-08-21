ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/swl/fd/fdmanager
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=        \
    camxfdmanagernode.cpp \
    camxfdutils.cpp       \
    camxfdasserts.cpp

LOCAL_INC_FILES :=      \
    camxfdmanagernode.h \
    camxfdutils.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                                       \
    $(CAMX_CHICDK_API_PATH)/fd                                               \
    $(CAMX_CHICDK_API_PATH)/generated/g_facedetection                        \
    $(CAMX_PATH)/src/hwl/titan17x                                            \
    $(CAMX_PATH)/src/hwl/fd                                                  \
    $(CAMX_PATH)/src/core                                                    \
    $(CAMX_PATH)/src/core/ncs                                                \
    $(CAMX_PATH)/src/swl/fd/fdmanager                                        \
    $(CAMX_OUT_HEADERS)/fd/fdengine                                          \
    $(CAMX_OUT_HEADERS)/swprocessalgo                                        \

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxswlfdmanager

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
