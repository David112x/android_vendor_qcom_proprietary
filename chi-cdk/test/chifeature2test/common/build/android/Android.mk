ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/test/chifeature2test
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_SRC_FILES :=                          \
    chifeature2testmain.cpp                 \

LOCAL_INC_FILES :=                          \

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES :=                                                 \
    $(CAMX_C_INCLUDES)                                              \
    $(LOCAL_PATH)/                                                  \
    $(LOCAL_PATH)/../chifeature2testframework/                      \
    $(CAMX_CHICDK_PATH)/../camx/src/core/hal/                       \
    $(CAMX_CHICDK_PATH)/../ext/                                     \
    $(CAMX_CHICDK_PATH)/../ext/hardware/                            \
    $(CAMX_CHICDK_PATH)/../ext/system/                              \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2anchorsync     \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2demux          \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2fusion         \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2generic        \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2graphselector  \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2hdr            \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2mfsr           \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2qcfa           \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2rawhdr         \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2rt             \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2serializer     \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2swmf           \
    $(TARGET_OUT_INTERMEDIATES)/include/common/inc                  \
    $(TARGET_OUT_INTERMEDIATES)/include/xmllib/inc                  \
    $(TARGET_OUT_HEADERS)/camx                                      \
    system/media/camera/include/system/                             \

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CFLAGS += -fno-exceptions         \
                -g                      \
                -Wno-unused-variable

LOCAL_CFLAGS += -DFEATURE_XMLLIB

LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Defining targets
ifeq ($(TARGET_BOARD_PLATFORM), kona)
    LOCAL_CFLAGS += -DTARGET_KONA
endif

ifeq ($(TARGET_BOARD_PLATFORM), msmnile)
    LOCAL_CFLAGS += -DTARGET_HANA
endif

ifeq ($(TARGET_BOARD_PLATFORM), sm6150)
    LOCAL_CFLAGS += -DTARGET_TALOSMOOREA
endif

ifeq ($(TARGET_BOARD_PLATFORM), sdm845)
    LOCAL_CFLAGS += -DTARGET_NAPALI
endif

# Defining Android platform for VGDB compiling
ifeq ($(ANDROID_FLAVOR), $(ANDROID_FLAVOR_Q))
    LOCAL_CFLAGS += -DPLATFORM_VERSION_Q
endif

# Libraries to statically link
LOCAL_STATIC_LIBRARIES :=           \
    libchifeature2                  \
    libchiframework                 \
    libchiusecase                   \
    libchiutils                     \
    libchifeature2testframework

# Libraries to statically link
LOCAL_WHOLE_STATIC_LIBRARIES :=     \
    libchifeature2                  \
    libchiframework                 \
    libchiusecase                   \
    libchiutils

# Libraries to link
LOCAL_SHARED_LIBRARIES +=                                   \
    com.qti.chi.override                                    \
    com.qti.feature2.gs                                     \
    com.qti.feature2.generic                                \
    com.qti.feature2.mfsr                                   \
    com.qti.feature2.rt                                     \
    libcamera_metadata                                      \
    libchilog                                               \
    libcutils                                               \
    libhardware                                             \
    libhidlbase                                             \
    liblog                                                  \
    libqdMetaData                                           \
    libsync                                                 \
    libutils                                                \
    libui                                                   \
    libxml                                                  \
    vendor.qti.hardware.camera.postproc@1.0-service-impl    \
    vendor.qti.hardware.vpp@1.1                             \
    vendor.qti.hardware.vpp@1.2                             \

LOCAL_HEADER_LIBRARIES +=           \
    display_headers

LOCAL_LDLIBS :=                 \
    -llog                       \
    -lz                         \
    -ldl

LOCAL_LDFLAGS :=

# Binary name
LOCAL_MODULE := chifeature2test

# Deployment path under bin
LOCAL_MODULE_RELATIVE_PATH := ../bin
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
