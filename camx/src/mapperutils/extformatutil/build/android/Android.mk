ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/mapperutils/extformatutil
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

# Generate header file
$(info $(shell perl $(LOCAL_PATH)/headerfilegen.pl $(CAMX_CHICDK_PATH)/oem/qcom/formatmapper/mapperutil.xml $(LOCAL_PATH)/g_formatmapper.h))

LOCAL_SRC_FILES :=                  \
    camxformatutilexternal.cpp      \

LOCAL_INC_FILES :=                  \
    camxformatutilexternal.h        \

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Libraries to link
# This is used by external module (gralloc), do not add any new shared libs
LOCAL_SHARED_LIBRARIES :=   \
    libcutils               \
    liblog                  \
    libsync                 \

LOCAL_LDLIBS := -lz

# Needed for Venus header files
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

# Binary name
LOCAL_MODULE := libcamxexternalformatutils

include $(CAMX_BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
LOCAL_COPY_HEADERS_TO := camx
LOCAL_COPY_HEADERS    := camxformatutilexternal.h

include $(BUILD_COPY_HEADERS)
