ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/core/chi
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=              \
    camxchi.cpp                 \
    camxchicomponent.cpp        \
    camxchicontext.cpp          \
    camxchinodewrapper.cpp      \
    camxchisession.cpp          \

LOCAL_INC_FILES :=              \
    camxchi.h                   \
    camxchicomponent.h          \
    camxchicontext.h            \
    camxchidefs.h               \
    camxchinodewrapper.h        \
    camxchisession.h            \
    camxchitypes.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)            \
    $(CAMX_CHICDK_API_PATH)/common                   \
    $(CAMX_CHICDK_API_PATH)/node                  \
    $(CAMX_CHICDK_API_PATH)/stats                 \
    $(CAMX_CHICDK_API_PATH)/pdlib                 \
    $(CAMX_CHICDK_API_PATH)/pdlibwrapper          \
    $(CAMX_CHICDK_API_PATH)/ncs                   \
    $(CAMX_PATH)/src/hwl/titan17x                 \
    $(CAMX_PATH)/src/core/hal                     \
    $(CAMX_PATH)/src/core/ncs                     \
    $(CAMX_PATH)/src/core                         \
    $(CAMX_OUT_HEADERS)/titan17x                  \
    $(CAMX_OUT_HEADERS)/titan48x                  \
    hardware/libhardware/include                  \
    system/core/include                           \
    system/media/camera/include                   \
    system/media/private/camera/include           \

LOCAL_C_INCLUDES += hardware/libhardware/include

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxchi

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
