ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/utils/log
endif
include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                     \
    camxchiofflinelogger.cpp           \

LOCAL_INC_FILES :=                     \

LOCAL_C_INCLUDES       := $(CAMX_C_INCLUDES)              \
                          $(CAMX_CHICDK_API_PATH)/utils   \
                          $(TARGET_OUT_HEADERS)/camx

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES := \
   libcutils    \
   liblog       \
   libsync

LOCAL_LDLIBS := -lz

LOCAL_MODULE           := libofflinelog

include $(CAMX_BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
LOCAL_COPY_HEADERS_TO := camx
LOCAL_COPY_HEADERS :=              \

include $(BUILD_COPY_HEADERS)



