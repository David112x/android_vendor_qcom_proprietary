ifeq ($(call is-vendor-board-platform,QCOM),true)
ifeq ($(strip $(AUDIO_FEATURE_ENABLED_AHAL_EXT)),true)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := vendor.qti.hardware.audiohalext-utils
LOCAL_MODULE_OWNER := qti
LOCAL_C_INCLUDES += $(LOCAL_PATH) \
                    $(LOCAL_PATH)/include
LOCAL_SRC_FILES := \
    ConfigStoreUtils.cpp

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidlbase \
    vendor.qti.hardware.audiohalext@1.0

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libbase \
                    libhidlbase \
                    vendor.qti.hardware.audiohalext@1.0

include $(BUILD_SHARED_LIBRARY)

endif # BUILD_TINY_ANDROID
endif # AUDIO_FEATURE_ENABLED_AHAL_EXT
endif # is-board-platform, QCOM
