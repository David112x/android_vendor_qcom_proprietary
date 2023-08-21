ifneq ($(strip $(USE_CAMERA_STUB)),true)

# Helper function to check SDK version
ifeq ($(CAMX_EXT_VBUILD),)
# Linux Build
CHECK_VERSION_LT = $(shell if [ $(1) -lt $(2) ] ; then echo true ; else echo false ; fi)
CHECK_VERSION_GE = $(shell if [ $(1) -ge $(2) ] ; then echo true ; else echo false ; fi)
endif # ifeq ($(CAMX_EXT_VBUILD),)

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

LOCAL_PATH:= $(call my-dir)
ifeq ($(CAMX_PATH),)
    CAMX_PATH := $(LOCAL_PATH)
else
    $(info CAMX_PATH already $(CAMX_PATH))
endif # ($(CAMX_PATH),)

include $(CLEAR_VARS)
LOCAL_MODULE       := android.hardware.camera.provider@2.4-service_64.rc
LOCAL_SRC_FILES    := $(LOCAL_MODULE)
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR_ETC)/init
include $(BUILD_PREBUILT)

# Take backup of SDLLVM specific flag and version defs as other modules (adreno)
# also maintain their own version of it.
OLD_SDCLANG_FLAG_DEFS    := $(SDCLANG_FLAG_DEFS)
OLD_SDCLANG_VERSION_DEFS := $(SDCLANG_VERSION_DEFS)

include $(CAMX_PATH)/build/infrastructure/android/autogen.mk
include $(CAMX_PATH)/src/chiiqutils/build/android/Android.mk
include $(CAMX_PATH)/src/core/build/android/Android.mk
include $(CAMX_PATH)/src/core/chi/build/android/Android.mk
include $(CAMX_PATH)/src/core/hal/build/android/Android.mk
include $(CAMX_PATH)/src/core/halutils/build/android/Android.mk
include $(CAMX_PATH)/src/csl/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/bps/build/android/Android.mk
ifeq ($(TARGET_BOARD_PLATFORM),kona)
include $(CAMX_PATH)/src/hwl/cvp/build/android/Android.mk
endif # kona
include $(CAMX_PATH)/src/hwl/fd/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/dspinterfaces/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/ife/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/ipe/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/iqsetting/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/isphwsetting/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/ispiqmodule/build/android/Android.mk
ifneq ($(IQSETTING),OEM1)
include $(CAMX_PATH)/src/hwl/iqinterpolation/build/android/Android.mk
endif
include $(CAMX_PATH)/src/hwl/jpeg/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/lrme/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/statsparser/build/android/Android.mk
include $(CAMX_PATH)/src/hwl/titan17x/build/android/Android.mk
include $(CAMX_PATH)/src/lib/build/android/Android.mk
include $(CAMX_PATH)/src/mapperutils/extformatutil/build/android/Android.mk
include $(CAMX_PATH)/src/mapperutils/formatmapper/build/android/Android.mk
include $(CAMX_PATH)/src/osutils/build/android/Android.mk
include $(CAMX_PATH)/src/settings/build/android/Android.mk
include $(CAMX_PATH)/src/swl/eisv2/build/android/Android.mk
include $(CAMX_PATH)/src/swl/eisv3/build/android/Android.mk
include $(CAMX_PATH)/src/swl/fd/fdmanager/build/android/Android.mk
include $(CAMX_PATH)/src/swl/jpeg/build/android/Android.mk
include $(CAMX_PATH)/src/swl/offlinestats/build/android/Android.mk
include $(CAMX_PATH)/src/swl/ransac/build/android/Android.mk
include $(CAMX_PATH)/src/swl/sensor/build/android/Android.mk
include $(CAMX_PATH)/src/swl/stats/build/android/Android.mk
include $(CAMX_PATH)/src/swl/swregistration/build/android/Android.mk
include $(CAMX_PATH)/src/utils/build/android/Android.mk
include $(CAMX_PATH)/src/utils/log/build/android/Android.mk
include $(CAMX_PATH)/src/core/ncs/build/android/Android.mk

# Restore previous value of sdllvm flag and version defs
SDCLANG_FLAG_DEFS    := $(OLD_SDCLANG_FLAG_DEFS)
SDCLANG_VERSION_DEFS := $(OLD_SDCLANG_VERSION_DEFS)

endif #!USE_CAMERA_STUB
