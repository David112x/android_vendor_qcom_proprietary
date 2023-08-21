# common.mk - Makefile for the CamX driver
#
# Things in this file are global to all CamX sub-projects. Consider adding things like
# include paths and linked libraries to individual sub-projects instead.
#
ifeq ($(CAMX_PATH),)
$(error CAMX_PATH should have been defined!)
endif # ($(CAMX_PATH),)

# Define path to chi-cdk directory
ifeq ($(CAMX_CHICDK_PATH),)
    CAMX_CHICDK_PATH := $(CAMX_PATH)/../chi-cdk
endif

# Define path to CamX header include directory in out directory
ifeq ($(CAMX_OUT_HEADERS),)
    CAMX_OUT_HEADERS := $(TARGET_OUT_HEADERS)/camx
endif

# Path prefix that can be overriden at compile time if mainline builds are used.
ifeq ($(CAMX_PATH_PREFIX),)
    CAMX_PATH_PREFIX := vendor/qcom/proprietary
endif

# Define path to common Android build files
ifeq ($(CAMX_BUILD_PATH),)
    CAMX_BUILD_PATH := $(CAMX_PATH)/build/infrastructure/android
endif

# Inherit definitions from CHI-CDK
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

# Set to enable function call profiling on gcc for supported modules
# ANDed with SUPPORTS_FUNCTION_CALL_TRACE in each makefile that includes common.mk
ENABLE_FUNCTION_CALL_TRACE := 0

# CAMX_OS can be linux or win32
CAMX_OS := linux

CAMX_LIB := camera.qcom

LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64

CAMX_LIB_OUTPUT_PATH := camera/components
CAMX_BIN_OUTPUT_PATH := camera

CAMX_CHICDK_API_PATH                 := $(CAMX_PATH_PREFIX)/chi-cdk/api
CAMX_CHICDK_CORE_PATH                := $(CAMX_PATH_PREFIX)/chi-cdk/core
CAMX_CHICDK_OEM_PATH                 := $(CAMX_PATH_PREFIX)/chi-cdk/oem
CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH   := $(CAMX_PATH_PREFIX)/chi-cdk/oem/qcom/feature2
CAMX_CHICDK_TOOLS_PATH               := $(CAMX_PATH_PREFIX)/chi-cdk/tools
CAMX_SYSTEM_PATH                     := $(CAMX_PATH_PREFIX)/camx-lib/system
CAMX_TDEV_PATH                       := $(CAMX_PATH_PREFIX)/camx-lib/test
CAMX_SYSTEM_STATS_PATH               := $(CAMX_PATH_PREFIX)/camx-lib-stats/system
CAMX_SYSTEM_3A_PATH                  := $(CAMX_PATH_PREFIX)/camx-lib-3a/system
CAMX_OEM1IQ_PATH                     := $(CAMX_PATH_PREFIX)/oemiq-ss/oem1iq
CAMX_EXT_PATH                        := $(CAMX_PATH_PREFIX)/ext
CAMX_TEST_PATH                       := $(CAMX_PATH_PREFIX)/test

# Inherit C Includes from CDK and add others
CAMX_C_INCLUDES :=                                                  \
    $(CAMX_C_INCLUDES)                                              \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2anchorsync     \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2demux          \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2fusion         \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2graphselector  \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2generic        \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2hdr            \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2mfsr           \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2qcfa           \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2rawhdr         \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2rt             \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2serializer     \
    $(CAMX_CHICDK_OEM_QCOM_FEATURE2_PATH)/chifeature2swmf           \
    $(CAMX_PATH)/src/core                                           \
    $(CAMX_PATH)/src/core/chi                                       \
    $(CAMX_PATH)/src/core/hal                                       \
    $(CAMX_PATH)/src/core/halutils                                  \
    $(CAMX_PATH)/src/csl                                            \
    $(CAMX_PATH)/src/mapperutils/formatmapper                       \
    $(CAMX_PATH)/src/osutils                                        \
    $(CAMX_PATH)/src/sdk                                            \
    $(CAMX_PATH)/src/settings/                                      \
    $(CAMX_PATH)/src/swl                                            \
    $(CAMX_PATH)/src/swl/jpeg                                       \
    $(CAMX_PATH)/src/swl/sensor                                     \
    $(CAMX_PATH)/src/swl/stats                                      \
    $(CAMX_PATH)/src/utils                                          \
    $(CAMX_PATH)/src/utils/scope                                    \
    $(CAMX_OUT_HEADERS)                                             \
    $(CAMX_OUT_HEADERS)/titan17x                                    \
    $(CAMX_OUT_HEADERS)/titan48x                                    \
    $(CAMX_OUT_HEADERS)/fd/fdengine                                 \
    $(CAMX_OUT_HEADERS)/swprocessalgo                               \
    $(CAMX_OUT_HEADERS)/localhistogramalgo

CAMX_C_INCLUDES := $(CAMX_C_INCLUDES) \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix

# Always include the system paths last
CAMX_C_INCLUDES += $(CAMX_SYSTEM_INCLUDES)

ifeq ($(IQSETTING),OEM1)
CAMX_CFLAGS +=                              \
    -DOEM1IQ=1
endif

ifeq ($(CAMX_PREBUILT_LINK),1)
LOCAL_LDFLAGS := $(CAMX_ADDITIONAL_LDFLAGS)
endif #ifeq ($(CAMX_PREBUILT_LINK),1)

# This is a hack to just override the -D flags the compiler passes into CAMX. Unfortunately,
# if you just change TARGET_BUILD_TYPE on the command line like you're "supposed" to, it
# will require a full Android debug build to link against the debug C libs..
ifneq ($(CAMXDEBUG),)
    # Add the flags we want. -Wno-psabi is due to a strange GCC warning that can't otherwise
    # be supressed.
    CAMX_CFLAGS += -DDEBUG -UNDEBUG -Wno-type-limits -Wno-sign-compare
endif # CAMXDEBUG

ifeq ($(CAMXMEMSPY),1)
    CAMX_CFLAGS += -DCAMX_USE_MEMSPY=1
endif # CAMXMEMSPY

CAMX_CFLAGS += -fcxx-exceptions
CAMX_CFLAGS += -fexceptions

ifeq ($(CAMX_EXT_VBUILD),)
# Linux build always uses SDLLVM
LOCAL_SDCLANG := true
endif

# Release builds vs debug builds
ifeq ($(CAMXDEBUG),)
    # Use the highest optimization level with SDLLVM and
    # use the latest version (>=4.0)
    ifeq ($(LOCAL_SDCLANG), true)
        LOCAL_SDCLANG_OFAST := true
        SDCLANG_FLAG_DEFS := $(CAMX_CHICDK_PATH)/core/build/android/sdllvm-flag-defs.mk
        SDCLANG_VERSION_DEFS := $(CAMX_CHICDK_PATH)/core/build/android/sdllvm-selection.mk
        -include $(SDCLANG_VERSION_DEFS)
    endif # ($(LOCAL_SDCLANG), true)
endif # ($(CAMXDEBUG),)

LOCAL_SHARED_LIBRARIES += libcdsprpc
LOCAL_SHARED_LIBRARIES += libqdMetaData
LOCAL_SHARED_LIBRARIES += libhardware

LOCAL_STATIC_LIBRARIES += libcamxgenerated

LOCAL_WHINER_RULESET := camx
CAMX_CHECK_WHINER := $(CAMX_BUILD_PATH)/check-whiner.mk

# Compile all HWLs in VGDB builds
ifneq ($(CAMX_EXT_VBUILD),)
CAMX_BUILD_EMULATED_SENSOR := 1
endif # ($(CAMX_EXT_VBUILD),)

# These are wrappers for commonly used build rules
CAMX_BUILD_STATIC_LIBRARY := $(CAMX_BUILD_PATH)/camx-build-static-library.mk
CAMX_BUILD_SHARED_LIBRARY := $(CAMX_BUILD_PATH)/camx-build-shared-library.mk
CAMX_BUILD_EXECUTABLE     := $(CAMX_BUILD_PATH)/camx-build-executable.mk

# Q and above
ifeq ($(call CHECK_VERSION_GE, $(PLATFORM_SDK_VERSION), $(PLATFORM_SDK_QPDK)), true)
ifeq ($(CAMX_EXT_VBUILD),)
CAMX_CPPFLAGS += -fdump-preamble
endif
endif
