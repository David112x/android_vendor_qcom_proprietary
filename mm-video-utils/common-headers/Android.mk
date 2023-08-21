VIDEO_DIR := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := libmmvideo_proprietary_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(VIDEO_DIR)/utils
LOCAL_EXPORT_C_INCLUDE_DIRS += $(VIDEO_DIR)/fastcrc
LOCAL_EXPORT_C_INCLUDE_DIRS += $(VIDEO_DIR)/streamparser
include $(BUILD_HEADER_LIBRARY)

