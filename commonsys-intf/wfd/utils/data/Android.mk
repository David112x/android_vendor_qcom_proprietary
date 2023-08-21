LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#            COPY THE WFDCONFIG.XML FILE TO /data
# ---------------------------------------------------------------------------------

LOCAL_MODULE:= wfdconfig.xml
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_ETC)
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_PREBUILT)

# ---------------------------------------------------------------------------------
#            COPY THE WFDCONFIGSINK.XML FILE TO /system/etc
# ---------------------------------------------------------------------------------
include $(CLEAR_VARS)
ifneq (,$(filter true, $(TARGET_FWK_SUPPORTS_FULL_VALUEADDS) $(TARGET_BOARD_AUTO)))
LOCAL_MODULE:= wfdconfigsink.xml
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
include $(BUILD_PREBUILT)
endif
