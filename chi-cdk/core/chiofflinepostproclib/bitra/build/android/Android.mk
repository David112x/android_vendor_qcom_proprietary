ifeq ($(CAMX_CHICDK_CORE_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_CORE_PATH := $(abspath $(LOCAL_PATH)/..)
else
LOCAL_PATH := $(CAMX_CHICDK_CORE_PATH)/chiofflinepostproclib
endif

include $(CLEAR_VARS)

# Get definitions common to the CHI-CDK project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_C_INCLUDES :=                                                 \
    $(CAMX_CHICDK_CORE_PATH)/lib/bitra                              \
    $(CAMX_C_INCLUDES)                                              \
    $(LOCAL_PATH)/                                                  \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2anchorsync     \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2generic        \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2graphselector  \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2hdr            \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2mfsr           \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2qcfa           \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2rt             \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2swmf           \

LOCAL_CFLAGS   := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)
LOCAL_C_LIBS   := $(CAMX_C_LIBS)

LOCAL_SHARED_LIBRARIES +=           \
    com.qti.chi.override.bitra      \
    com.qti.feature2.generic.bitra  \
    com.qti.feature2.gs.bitra       \
    libcamera_metadata              \
    libchilog                       \
    liblog                          \
    libqdMetaData                   \
    libsync                         \


LOCAL_SRC_FILES :=                  \
    chiofflinejpegencode.cpp        \
    chiofflinepostprocencode.cpp    \
    chiofflinepostprocintf.cpp      \


LOCAL_INC_FILES :=                  \
    chiofflinejpegencode.h          \
    chiofflinepostprocbase.h        \
    chiofflinepostprocencode.h      \
    chiofflinepostprocintf.h        \
    chiofflinepostproctypes.h       \


LOCAL_LDLIBS := \
    -llog       \
    -lz         \
    -ldl

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libcamerapostproc.bitra

include $(BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)
