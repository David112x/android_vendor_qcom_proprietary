LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-subdir-java-files, java)

ifeq ($(TARGET_PRODUCT),qssi_32)
LOCAL_MANIFEST_FILE := qssi_32/AndroidManifest.xml
else
ifeq ($(TARGET_HAS_LOW_RAM), true)
LOCAL_MANIFEST_FILE := qssi_32/AndroidManifest.xml
else
LOCAL_MANIFEST_FILE := AndroidManifest.xml
endif
endif

LOCAL_MODULE_TAGS := optional
LOCAL_PACKAGE_NAME := UnifiedSensorTestAppService
LOCAL_JAVA_LIBRARIES := android.hidl.base-V1.0-java android.hidl.manager-V1.0-java
LOCAL_STATIC_JAVA_LIBRARIES := UnifiedSensorTestAppServiceManager
LOCAL_CERTIFICATE := platform
LOCAL_PRIVATE_PLATFORM_APIS := true
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
