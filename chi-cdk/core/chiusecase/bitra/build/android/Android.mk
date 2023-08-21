ifeq ($(CAMX_CHICDK_CORE_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_CORE_PATH := $(abspath $(LOCAL_PATH)/..)
else
LOCAL_PATH := $(CAMX_CHICDK_CORE_PATH)/chiusecase
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES :=                      \
    chxadvancedcamerausecase.h          \
    chxusecasedefault.h                 \
    chxusecasemc.h                      \
    chxusecasevrmc.h                    \
    chxusecasesuperslowmotionfrc.h      \
    chxusecasetorch.h                   \
    chxusecaseutils.h

LOCAL_SRC_FILES :=                                \
    chxadvancedcamerausecase.cpp                  \
    chxusecasedefault.cpp                         \
    chxusecasemc.cpp                              \
    chxusecasevrmc.cpp                            \
    chxusecasesuperslowmotionfrc.cpp              \
    chxusecasetorch.cpp                           \
    chxusecaseutils.cpp

ifeq ($(call CHECK_VERSION_GE, $(PLATFORM_SDK_VERSION), $(PLATFORM_SDK_PPDK)), true)
LOCAL_INC_FILES +=               \
    chxusecasedual.h
LOCAL_SRC_FILES +=               \
    chxusecasedual.cpp
endif

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                                                    \
    $(CAMX_CHICDK_CORE_PATH)/lib/bitra                                 \
    $(CAMX_C_INCLUDES)                                                 \
    $(CAMX_CHICDK_API_PATH)/utils                                      \
    $(CAMX_CHICDK_PATH)/oem/qcom/feature2/chifeature2graphselector

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES +=           \
    libchilog                       \
    vendor.qti.hardware.vpp@1.1     \
    vendor.qti.hardware.vpp@1.2     \

LOCAL_HEADER_LIBRARIES +=           \
    display_headers

LOCAL_LDFLAGS :=
LOCAL_LDLIBS :=

LOCAL_MODULE := libchiusecase.bitra

include $(BUILD_STATIC_LIBRARY)

-include $(CAMX_CHECK_WHINER)
