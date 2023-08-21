ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/core/ncs
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=             \
    camxncsservice.cpp         \
    camxncssensor.cpp          \
    camxncssensordata.cpp      \
    camxncsintfqsee.cpp        \
    camxtofsensorintf.cpp      \
    camxncssscutils.cpp        \
    camxncssscconnection.cpp

LOCAL_INC_FILES :=           \
    camxncsintf.h            \
    camxncsservice.h         \
    camxncssensor.h          \
    camxncssensordata.h      \
    camxncsintfqsee.h        \
    camxtofsensorintf.h      \
    camxncssscutils.h        \
    camxncssscconnection.h   \
    sns_client_api_v01.h


# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)       \
    hardware/libhardware/include             \
    system/media/camera/include              \
    $(CAMX_CHICDK_API_PATH)/common/          \
    $(CAMX_CHICDK_API_PATH)/pdlib/           \
    $(CAMX_CHICDK_API_PATH)/generated/g_sensor/   \
    $(TARGET_OUT_HEADERS)/qmi-framework/inc  \
    $(TARGET_OUT_HEADERS)/sensors/nanopb/inc \
    $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libsnsapi_intermediates/proto

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_CFLAGS += -Werror -Wall -Wno-unused-parameter -fexceptions

LOCAL_SHARED_LIBRARIES =            \
    libutils                        \
    libprotobuf-cpp-full            \
    libsensorslog                   \
    libqmi_common_so                \
    libqmi_cci                      \
    libqmi_encdec                   \
    libsnsapi                       \
    libhardware                     \
    libcutils                       \
    libofflinelog                   \


LOCAL_STATIC_LIBRARIES =            \

ifneq ($(ANDROID_FLAVOR), $(ANDROID_FLAVOR_P))
ifneq ($(ANDROID_FLAVOR), $(ANDROID_FLAVOR_Q))
LOCAL_HEADER_LIBRARIES += libnanopb_headers
LOCAL_HEADER_LIBRARIES += sns_proto_intermediate_headers
LOCAL_HEADER_LIBRARIES += libqmi_common_headers
endif # ifneq ANDROID_FLAVOR_Q
endif # ifneq ANDROID_FLAVOR_P

LOCAL_WHOLE_STATIC_LIBRARIES :=     \
    libcamxosutils                  \
    libcamxutils                    \

LOCAL_LDLIBS := -lz -llog

# Binary name
LOCAL_MODULE := libcamxncs


include $(CAMX_BUILD_SHARED_LIBRARY)
-include $(CAMX_CHECK_WHINER)
