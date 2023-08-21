ifneq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(CAMX_CHICDK_PATH)/core
else
LOCAL_PATH := $(call my-dir)
endif

ifeq ($(TARGET_BOARD_PLATFORM),lito)
include $(CAMX_CHICDK_CORE_PATH)/chifeature2/bitra/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiframework/bitra/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiofflinepostproclib/bitra/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiofflinepostprocservice/bitra/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiusecase/bitra/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiutils/bitra/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/lib/bitra/build/android/Android.mk
endif

include $(CAMX_CHICDK_CORE_PATH)/chifeature2/common/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiframework/common/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiofflinepostproclib/common/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiofflinepostprocservice/common/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiusecase/common/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/chiutils/common/build/android/Android.mk
include $(CAMX_CHICDK_CORE_PATH)/lib/common/build/android/Android.mk
