ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/core/hal
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=              \
    camxentry.cpp               \
    camxhal3.cpp                \
    camxhal3entry.cpp           \
    camxhal3metadatatags.cpp    \
    camxhal3module.cpp          \
    camxhal3stream.cpp          \
    camxhal3types.cpp           \
    camxhaldevice.cpp           \
    camxpresilmem.cpp           \
    camxthermalmanager.cpp      \

LOCAL_INC_FILES :=              \
    camxentry.h                 \
    camxhal3.h                  \
    camxhal3defs.h              \
    camxhal3entry.h             \
    camxhal3metadatatags.h      \
    camxhal3metadatatagtypes.h  \
    camxhal3module.h            \
    camxhal3stream.h            \
    camxhal3types.h             \
    camxhaldevice.h             \
    camxthermalmanager.h        \
    camxpresilmem.h             \

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)      \
    $(CAMX_CHICDK_API_PATH)/common          \
    $(CAMX_CHICDK_API_PATH)/utils           \
    $(CAMX_PATH)/src/core/ncs               \
    hardware/libhardware/include            \
    system/core/include                     \
    system/media/camera/include             \
    system/media/private/camera/include     \

LOCAL_C_INCLUDES += hardware/libhardware/include

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxhal

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
LOCAL_COPY_HEADERS_TO := camx
LOCAL_COPY_HEADERS :=  \
    camxhal3types.h             \
    camxcommontypes.h           \
    camxpresilmem.h

include $(BUILD_COPY_HEADERS)
