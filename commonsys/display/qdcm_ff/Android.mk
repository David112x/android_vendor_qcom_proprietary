LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under,src)

LOCAL_STATIC_JAVA_LIBRARIES := \
    vendor.display.color-V1.1-java
LOCAL_JAVA_LIBRARIES := android.hidl.base-V1.0-java android.hidl.manager-V1.0-java

LOCAL_PACKAGE_NAME := QdcmFF
LOCAL_DEX_PREOPT   := false
LOCAL_CERTIFICATE  := platform
LOCAL_MODULE_OWNER := qti
LOCAL_PRODUCT_MODULE := true
LOCAL_PRIVATE_PLATFORM_APIS := true

include $(BUILD_PACKAGE)
