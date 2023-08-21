ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2graphselector
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CHI-CDK project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_SRC_FILES :=                                           \
   chifeature2anchorsyncdescriptor.cpp                       \
   chifeature2bayer2yuvdescriptor.cpp                        \
   chifeature2demuxdescriptor.cpp                            \
   chifeature2frameselectdescriptor.cpp                      \
   chifeature2fusiondescriptor.cpp                           \
   chifeature2formatconvertordescriptor.cpp                  \
   chifeature2bokehdescriptor.cpp                            \
   chifeature2bpsdescriptor.cpp                              \
   chifeature2graphselector.cpp                              \
   $(TARGET_BOARD_PLATFORM)/chifeature2graphselectoroem.cpp  \
   chifeature2graphdescriptors.cpp                           \
   chifeature2ipedescriptor.cpp                              \
   chifeature2jpegdescriptor.cpp                             \
   chifeature2mfnrdescriptor.cpp                             \
   chifeature2mfsrdescriptor.cpp                             \
   chifeature2oemgraphdescriptors.cpp                        \
   chifeature2realtimedescriptor.cpp                         \
   chifeature2hdrtype1descriptor.cpp                         \
   chifeature2rawhdrdescriptor.cpp                           \
   chifeature2serializerdescriptor.cpp                       \
   chifeature2swmfdescriptor.cpp                             \
   chifeature2memcpydescriptor.cpp

LOCAL_INC_FILES :=                  \
   chifeature2graphdescriptors.h    \
   chifeature2graphselector.h       \
   chifeature2graphselectoroem.h    \
   chifeature2graphselectortable.h

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                                      \
    $(CAMX_C_INCLUDES)                                   \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chiframework   \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chiusecase     \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chiutils


# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_WHOLE_STATIC_LIBRARIES +=     \
    libchiutils                     \

LOCAL_SHARED_LIBRARIES +=           \
    com.qti.chi.override            \
    libcamera_metadata              \
    libchilog                       \
    libcutils                       \
    libhardware                     \
    libhidlbase                     \
    liblog                          \
    libqdMetaData                   \
    libsync                         \
    libutils                        \
    vendor.qti.hardware.vpp@1.1     \
    vendor.qti.hardware.vpp@1.2

# Binary name
LOCAL_MODULE := com.qti.feature2.gs

LOCAL_LDLIBS := -lz

include $(BUILD_SHARED_LIBRARY)

-include $(CAMX_CHECK_WHINER)
