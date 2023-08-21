ifeq ($(CAMX_CHICDK_CORE_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_CORE_PATH := $(abspath $(LOCAL_PATH)/..)
else
LOCAL_PATH := $(CAMX_CHICDK_CORE_PATH)/chifeature2
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES :=                      \
    chifeature2base.h                   \
    chifeature2featurepool.h            \
    chifeature2graph.h                  \
    chifeature2graphmanager.h           \
    chifeature2requestobject.h          \
    chifeature2types.h                  \
    chifeature2usecaserequestobject.h   \
    chifeature2utils.h                  \
    chifeature2wrapper.h                \
    chitargetbuffermanager.h            \
    chithreadmanager.h

LOCAL_SRC_FILES :=                                \
    chifeature2base.cpp                           \
    chifeature2baserequestflow.cpp                \
    chifeature2featurepool.cpp                    \
    chifeature2graph.cpp                          \
    chifeature2graphmanager.cpp                   \
    chifeature2multistagedescriptor.cpp           \
    chifeature2requestobject.cpp                  \
    chifeature2usecaserequestobject.cpp           \
    chifeature2wrapper.cpp                        \
    chitargetbuffermanager.cpp                    \
    chithreadmanager.cpp

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                                                    \
    $(CAMX_C_INCLUDES)                                                 \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2graphselector



# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_LDFLAGS :=
LOCAL_LDLIBS :=

LOCAL_SHARED_LIBRARIES := \
    libcutils    \
    libchilog    \
    liblog       \
    libsync

LOCAL_MODULE := libchifeature2

include $(BUILD_STATIC_LIBRARY)

-include $(CAMX_CHECK_WHINER)
