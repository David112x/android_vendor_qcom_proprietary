ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/chiiqutils
endif
include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                  \
    camxchiiqutilshandler.cpp
LOCAL_INC_FILES :=                  \
    camxchiiqutilshandler.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)              \
    $(CAMX_CHICDK_API_PATH)/utils                   \
    $(TARGET_OUT_HEADERS)/camx

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES +=      \
    libchilog                  \
    libcom.qti.chinodeutils

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libcamxosutils              \
    libcamxutils

LOCAL_STATIC_LIBRARIES :=       \
    libnc

LOCAL_LDLIBS := -lz

# Binary name
LOCAL_MODULE := com.qti.camx.chiiqutils

LOCAL_MODULE_RELATIVE_PATH := $(CAMX_LIB_OUTPUT_PATH)

include $(CAMX_BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
#LOCAL_COPY_HEADERS_TO := camx
#LOCAL_COPY_HEADERS := camxchiiqutilshandler.h               \

#include $(BUILD_COPY_HEADERS)
