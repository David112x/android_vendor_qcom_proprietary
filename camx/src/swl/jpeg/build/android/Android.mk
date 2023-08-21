ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/swl/jpeg
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=            \
    camxjpegaggrnode.cpp      \
    camxjpegexifcomposer.cpp  \
    camxjpegexifparams.cpp    \
    camxjpeghufftable.cpp     \
    camxjpegquanttable.cpp    \
    camxjpegutil.cpp

LOCAL_INC_FILES :=            \
    camxjpegaggrnode.h        \
    camxjpegexifcomposer.h    \
    camxjpegexifdefs.h        \
    camxjpegexifdefaults.h    \
    camxjpegexifparams.h      \
    camxjpeghufftable.h       \
    camxjpegquanttable.h      \
    camxjpegutil.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)            \
    $(CAMX_PATH)/src/core                         \
    $(CAMX_PATH)/src/swl/jpeg                     \

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxswljpeg

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
