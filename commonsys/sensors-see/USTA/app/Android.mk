LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := UnifiedSensorTestApp
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES += $(call all-java-files-under, java)

LOCAL_STATIC_JAVA_LIBRARIES := UnifiedSensorTestAppServiceManager

ifeq ($(PLATFORM_VERSION), P)
LOCAL_AAPT2_ONLY := true
endif

LOCAL_RESOURCE_DIR = $(LOCAL_PATH)/res

ifeq ($(PLATFORM_VERSION), P)
LOCAL_RESOURCE_DIR += frameworks/support/core-ui/res
endif

LOCAL_AAPT_FLAGS := \
        --auto-add-overlay \
        --no-version-vectors
LOCAL_STATIC_ANDROID_LIBRARIES := \
        android-support-design

LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PROGUARD_ENABLED := disabled
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))

