# @file Android.mk
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under,src)
LOCAL_REQUIRED_MODULES := \
    com.qti.snapdragon.sdk.display
LOCAL_STATIC_JAVA_LIBRARIES := vendor.display.color-V1.0-java
LOCAL_JAVA_LIBRARIES := \
    com.qti.snapdragon.sdk.display android.hidl.base-V1.0-java \
    android.hidl.manager-V1.0-java
LOCAL_PACKAGE_NAME := colorservice
LOCAL_DEX_PREOPT := false
LOCAL_PROGUARD_ENABLED := disabled
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_OWNER := qti
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PRODUCT_MODULE := true
include $(BUILD_PACKAGE)
