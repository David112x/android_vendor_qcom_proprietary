ifeq ($(CAMX_CHICDK_CORE_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_CORE_PATH := $(abspath $(LOCAL_PATH)/..)
else
LOCAL_PATH := $(CAMX_CHICDK_CORE_PATH)/chiofflinepostprocservice
endif

include $(CLEAR_VARS)

# Get definitions common to the CHI-CDK project here
# For VGDB builds, following external libraries needs to be included
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_C_INCLUDES :=     \
    $(CAMX_C_INCLUDES)  \
    $(LOCAL_PATH)/      \

LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS) \
                  -std=c++17
LOCAL_LDLIBS   := -lz
LOCAL_CFLAGS   := $(CAMX_CFLAGS)
LOCAL_C_LIBS   := $(CAMX_C_LIBS)

LOCAL_SHARED_LIBRARIES +=                   \
    android.hardware.graphics.mapper@2.0    \
    android.hardware.graphics.mapper@3.0    \
    libcamera_metadata                      \
    libhidlbase                             \
    liblog                                  \
    libutils                                \
    vendor.qti.hardware.camera.postproc@1.0 \

LOCAL_INC_FILES :=          \
    postprocservice.h       \
    postprocserviceintf.h   \
    postprocsession.h       \

LOCAL_SRC_FILES :=          \
    postprocservice.cpp     \
    postprocserviceintf.cpp \
    postprocsession.cpp     \

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := vendor.qti.hardware.camera.postproc@1.0-service-impl

include $(BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)
