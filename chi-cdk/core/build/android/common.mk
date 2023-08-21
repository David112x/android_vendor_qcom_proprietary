# common.mk - Makefile for Chi-CDK
#
# Things in this file are global to all Chi-CDK sub-projects. Consider adding things like
# include paths and linked libraries to individual sub-projects instead.
#

# Set to enable function call profiling on gcc for supported modules
# ANDed with SUPPORTS_FUNCTION_CALL_TRACE in each makefile that includes common.mk
ENABLE_FUNCTION_CALL_TRACE := 0

# CAMX_OS can be linux or win32
CAMX_OS := linux

# Sanity
ifeq ($(CAMX_CHICDK_PATH),)
$(error CAMX_CHICDK_PATH must be defined before calling this makefile!)
endif

# Make sure we have default values
ifeq ($(CAMX_CHICDK_API_PATH),)
    CAMX_CHICDK_API_PATH := $(CAMX_CHICDK_PATH)/api
endif
ifeq ($(CAMX_CHICDK_CORE_PATH),)
    CAMX_CHICDK_CORE_PATH := $(CAMX_CHICDK_PATH)/core
endif
ifeq ($(CAMX_CHICDK_OEM_PATH),)
    CAMX_CHICDK_OEM_PATH := $(CAMX_CHICDK_PATH)/oem
endif
ifeq ($(CAMX_OUT_HEADERS),)
    CAMX_OUT_HEADERS := $(TARGET_OUT_HEADERS)/camx
endif
ifeq ($(CAMX_PATH_PREFIX),)
    CAMX_PATH_PREFIX := vendor/qcom/proprietary
endif
ifeq ($(CAMX_BUILD_PATH),)
    CAMX_BUILD_PATH := $(CAMX_PATH_PREFIX)/camx/build/infrastructure/android
endif
ifeq ($(CAMX_OEM1IQ_PATH),)
    CAMX_OEM1IQ_PATH := $(CAMX_PATH_PREFIX)/oemiq-ss/oem1iq
endif

CAMX_C_INCLUDES :=                                          \
    external/zlib                                           \
    external/openssl/include                                \
    hardware/libhardware/include/hardware                   \
    system/core/include                                     \
    system/core/libsync/include                             \
    system/media/camera/include                             \
    system/media/private/camera/include                     \
    $(CAMX_CHICDK_API_PATH)/common                          \
    $(CAMX_CHICDK_API_PATH)/fd                              \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix           \
    $(CAMX_CHICDK_API_PATH)/generated/g_facedetection       \
    $(CAMX_CHICDK_API_PATH)/generated/g_parser              \
    $(CAMX_CHICDK_API_PATH)/generated/g_sensor              \
    $(CAMX_CHICDK_API_PATH)/isp                             \
    $(CAMX_CHICDK_API_PATH)/ncs                             \
    $(CAMX_CHICDK_API_PATH)/node                            \
    $(CAMX_CHICDK_API_PATH)/pdlib                           \
    $(CAMX_CHICDK_API_PATH)/sensor                          \
    $(CAMX_CHICDK_API_PATH)/stats                           \
    $(CAMX_CHICDK_API_PATH)/utils                           \
    $(CAMX_CHICDK_CORE_PATH)/chifeature/                    \
    $(CAMX_CHICDK_CORE_PATH)/chifeature2/                   \
    $(CAMX_CHICDK_CORE_PATH)/chiframework                   \
    $(CAMX_CHICDK_CORE_PATH)/chiofflinepostproclib          \
    $(CAMX_CHICDK_CORE_PATH)/chiofflinepostprocservice      \
    $(CAMX_CHICDK_CORE_PATH)/chiusecase                     \
    $(CAMX_CHICDK_CORE_PATH)/chiutils                       \
    $(CAMX_CHICDK_CORE_PATH)/lib/common                     \
    $(CAMX_OUT_HEADERS)                                     \
    $(TARGET_OUT_HEADERS)

# Gralloc includes must remain in this order due to multiple copies of gralloc_priv.h
CAMX_C_INCLUDES +=                                          \
    vendor/qcom/opensource/commonsys-intf/display/gralloc   \
    hardware/qcom/display/gralloc

ifeq ($(IQSETTING),OEM1)
LOCAL_C_INCLUDES :=                             \
    $(CAMX_OEM1IQ_PATH)/chromatix/g_chromatix
else
LOCAL_C_INCLUDES :=                             \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix
endif

# Always include the system paths last
CAMX_C_INCLUDES += $(CAMX_SYSTEM_INCLUDES)

# Put here any libraries that should be linked by Chi-CDK projects
CAMX_C_LIBS :=

# VGDB build specific flags that must come first as they clobber warnings
ifneq ($(CAMX_EXT_VBUILD),)
CAMX_CFLAGS :=                  \
    $(CAMX_ADDITIONAL_CFLAGS)
else
CAMX_CFLAGS :=
endif # ($(CAMX_EXT_VBUILD),)

# Common CFLags to the project
CAMX_CFLAGS +=                              \
    $(DWARN_ALWAYS_ON)                      \
    $(DLINUX_FLAGS)                         \
    $(CFLAGS)                               \
    -DCAMX                                  \
    -D_LINUX                                \
    -DCAMX_LOGS_ENABLED=1                   \
    -DCAMX_TRACES_ENABLED=1                 \
    -fPIC                                   \
    -Werror                                 \
    -Wno-missing-field-initializers

# Release builds should hide symbols
ifeq ($(CAMXDEBUG),)
CAMX_CFLAGS += -fvisibility=hidden
endif #CAMXDEBUG

ifneq ($(LOCAL_CLANG),false)
CAMX_CFLAGS += -Wno-implicit-exception-spec-mismatch
else # ($(LOCAL_CLANG),false)
# Increase function alignment for better and more repeatable performance
CAMX_CFLAGS += -falign-functions=32
endif # ($(LOCAL_CLANG),false)

# -Wdate-time only support in gcc 4.9 and above.
ifeq ($(filter 4.6 4.6.% 4.7 4.7.% 4.8 4.8.%, $(TARGET_GCC_VERSION)),)
# 8994 L release use gcc 4.9 but don't have date-time support.
ifneq ($(filter -Werror=date-time,$(TARGET_ERROR_FLAGS)),)
CAMX_CFLAGS += -Wno-error=date-time
endif # TARGET_ERROR_FLAGS
endif # TARGET_GCC_VERSION

# 64bit builds generate many type punning warnings which suggests possible bugs
# Disable strict aliasing optimizations. Note that warning cannot be suppressed with -Wno-strict-aliasing.
CAMX_CFLAGS += -fno-strict-aliasing

# VGDB build specific C++ flags that must come first as they clobber warnings
ifneq ($(CAMX_EXT_VBUILD),)
CAMX_CPPFLAGS :=                  \
    $(CAMX_ADDITIONAL_CPPFLAGS)
else
CAMX_CPPFLAGS :=
endif # ($(CAMX_EXT_VBUILD),)

CAMX_CPPFLAGS +=                \
    -Wno-invalid-offsetof       \
    -std=c++17

ifeq ($(CAMX_EXT_VBUILD),)
    # Linux build always uses SDLLVM
    LOCAL_SDCLANG := true

    # Force ARM mode here. The driver will build as THUMB2 in 32bit release builds but we want to
    # link with the ARM libs to match the Linux based builds. Improves run to run variation.
    LOCAL_ARM_MODE := arm
endif

# Release builds vs debug builds
ifeq ($(CAMXDEBUG),)
    # Use the highest optimization level with SDLLVM and
    # use the latest version (>=4.0)
    ifeq ($(LOCAL_SDCLANG), true)
        LOCAL_SDCLANG_OFAST := true
        SDCLANG_FLAG_DEFS := $(CAMX_CHICDK_CORE_PATH)/build/android/sdllvm-flag-defs.mk
        # Select latest version of SDLLVM (>=4.0).
        SDCLANG_VERSION_DEFS := $(CAMX_CHICDK_CORE_PATH)/build/android/sdllvm-selection.mk
        -include $(SDCLANG_VERSION_DEFS)
    endif # ($(LOCAL_SDCLANG), true)
endif # ($(CAMXDEBUG),)

# Optimization flags
ifneq ($(CAMX_EXT_VBUILD),)

ifeq ($(CAMXDEBUG),)
CAMX_CFLAGS += -O0
else # ($(CAMXDEBUG),)
CAMX_CFLAGS += -O0
endif # ($(CAMXDEBUG),)

else # ($(CAMX_EXT_VBUILD),)

ifeq ($(CAMXDEBUG),)
# @todo (CAMX-2029) Need to change this back to O3 once we fix why it fails
CAMX_CFLAGS += -O0
else # ($(CAMXDEBUG),)
CAMX_CFLAGS += -O0
endif # ($(CAMXDEBUG),)

endif # ($(CAMX_EXT_VBUILD),)

# Describes what type of thread specific objects we want to have
# By default we will use POSIX style TLS objects using pthread_key create/delete APIs.
# A few pros in using this style of TLS objects are -
#   - Complete control over the life cycle of TLS objects.
#   - Provides compiler independent code, specifically, this is
#     necessary to compile modules while using LLVM (clang) compiler
# The other option is to use the compiler dependent __thread data type.
ENABLE_PTHREAD_TLS := 1
ifeq ($(ENABLE_PTHREAD_TLS),1)
CAMX_CFLAGS += -DPTHREAD_TLS
endif

ifeq ($(ENABLE_FUNCTION_CALL_TRACE),1)
    ifeq ($(SUPPORT_FUNCTION_CALL_TRACE),1)
        # -finstrument-functions    Add calls to __cyg_profile_func_enter()/__cyg_profile_func_exit()
        #                           around function calls
        # -mpoke-function-name      Embed the name of each function above the function itself
        CAMX_CFLAGS += -finstrument-functions -mpoke-function-name -DFUNCTION_CALL_TRACE
    endif #ifeq ($(SUPPORT_FUNCTION_CALL_TRACE),1)
endif #ifeq ($(ENABLE_FUNCTION_CALL_TRACE),1)

CAMX_CFLAGS += -DCAMX_ANDROID_API=$(PLATFORM_SDK_VERSION)
CAMX_CPPFLAGS += -DCAMX_ANDROID_API=$(PLATFORM_SDK_VERSION)

# Defaults projects are unlikely to change (@todo: what is local module relative path?)
LOCAL_SHARED_LIBRARIES := libc libc++ libcutils libdl liblog libofflinelog libsync
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_RELATIVE_PATH :=

# For __android_log
LOCAL_LDLIBS += -llog

LOCAL_HEADER_LIBRARIES :=

# P and above
ifeq ($(call CHECK_VERSION_GE, $(PLATFORM_SDK_VERSION), $(PLATFORM_SDK_PPDK)), true)
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += libcutils_headers
LOCAL_HEADER_LIBRARIES += libhardware_headers
endif

# Q and above
ifeq ($(call CHECK_VERSION_GE, $(PLATFORM_SDK_VERSION), $(PLATFORM_SDK_QPDK)), true)
ifeq ($(CAMX_EXT_VBUILD),)
CAMX_CPPFLAGS += -fdump-preamble
endif
endif

LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64

CAMX_LIB_OUTPUT_PATH := camera/components
CAMX_FEATURE2_LIB_OUTPUT_PATH := camera/components/feature2/qti
CAMX_BIN_OUTPUT_PATH := camera

# Whiner definitions
LOCAL_WHINER_RULESET := camx
CAMX_CHECK_WHINER := $(CAMX_BUILD_PATH)/check-whiner.mk
