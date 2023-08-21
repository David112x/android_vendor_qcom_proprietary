ifneq ($(strip $(USE_CAMERA_STUB)),true)

# Helper function to check SDK version
ifeq ($(CAMX_EXT_VBUILD),)
# Linux Build
CHECK_VERSION_LT = $(shell if [ $(1) -lt $(2) ] ; then echo true ; else echo false ; fi)
CHECK_VERSION_GE = $(shell if [ $(1) -ge $(2) ] ; then echo true ; else echo false ; fi)
endif # ($(CAMX_EXT_VBUILD),)

# Abstract the use of PLATFORM_VERSION to detect the version of Android.
# On the Linux NDK, a build error is seen when a character such as 'Q'
# is used; it must be an integer. On Windows NDK it may be an integer
# or a character. Linux AU builds provide a character for this variable.
# Therefore, use ANDROID_FLAVOR for everything. ANDROID_FLAVOR is defined
# in the build configuration files for the NDK environments. For Linux
# AUs, map value received from the environment to our own internal
# ANDROID_FLAVOR.
ifeq ($(ANDROID_FLAVOR_Q),)
ANDROID_FLAVOR_Q = Q
endif # ($(ANDROID_FLAVOR_Q),)

ifeq ($(CAMX_EXT_VBUILD),)
    ifeq ($(PLATFORM_VERSION), Q)
        ifeq ($(ANDROID_FLAVOR),)
            ANDROID_FLAVOR := $(ANDROID_FLAVOR_Q)
        endif # ($(ANDROID_FLAVOR),)
    endif # ($(PLATFORM_VERSION), Q)

endif # ($(CAMX_EXT_LNDKBUILD),)

# Map SDK/API level to Android verisons
PLATFORM_SDK_PPDK = 28
PLATFORM_SDK_QPDK = 29


# Determine the proper SDK/API level for the given Android version
ifeq ($(PLATFORM_SDK_VERSION),)
    ifeq ($(ANDROID_FLAVOR), $(ANDROID_FLAVOR_Q))
        PLATFORM_SDK_VERSION := $(PLATFORM_SDK_QPDK)
    endif # ($(ANDROID_FLAVOR), $(ANDROID_FLAVOR_Q))

endif # ($(PLATFORM_SDK_VERSION),)

CAMX_CHICDK_PATH := $(call my-dir)
CAMX_CHICDK_API_PATH := $(CAMX_CHICDK_PATH)/api
CAMX_CHICDK_CORE_PATH := $(CAMX_CHICDK_PATH)/core
CAMX_CHICDK_OEM_PATH := $(CAMX_CHICDK_PATH)/oem
CAMX_CHICDK_TEST_PATH := $(CAMX_CHICDK_PATH)/test
CAMX_CHICDK_TOPOLOGY_PATH := $(CAMX_CHICDK_OEM_PATH)/qcom/topology/

# Take backup of SDLLVM specific flag and version defs as other modules (adreno)
# also maintain their own version of it.
OLD_SDCLANG_FLAG_DEFS    := $(SDCLANG_FLAG_DEFS)
OLD_SDCLANG_VERSION_DEFS := $(SDCLANG_VERSION_DEFS)

include $(CAMX_CHICDK_API_PATH)/generated/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/Android.mk
include $(CAMX_CHICDK_OEM_PATH)/Android.mk
include $(CAMX_CHICDK_TEST_PATH)/Android.mk

# Restore previous value of sdllvm flag and version defs
SDCLANG_FLAG_DEFS    := $(OLD_SDCLANG_FLAG_DEFS)
SDCLANG_VERSION_DEFS := $(OLD_SDCLANG_VERSION_DEFS)

endif #!USE_CAMERA_STUB
