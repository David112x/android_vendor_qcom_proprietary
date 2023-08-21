LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := init.qcom.display.sh
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := init.qcom.display.sh
LOCAL_MODULE_PATH  := $(TARGET_OUT)/bin
LOCAL_INIT_RC := init.qcom.display.rc
include $(BUILD_PREBUILT)
