CUR_DIR := $(CUR_DIR) $(call my-dir)
LOCAL_PATH := $(lastword $(CUR_DIR))

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../common.mk

VPP_TARGET_DIR := $(LOCAL_BOARD_PLATFORM)
ifeq ($(LOCAL_BOARD_PLATFORM),$(MSMSTEPPE))
VPP_TARGET_DIR := msmsteppe
endif

#=============================================================================
# VPP Core Header Library
#=============================================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libvppheaders
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/core
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
include $(BUILD_HEADER_LIBRARY)

#=============================================================================
# DSP Header Library
#=============================================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libvppdspheaders
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/common
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
include $(BUILD_HEADER_LIBRARY)

#=============================================================================
# HCP Header Library
#=============================================================================
ifeq "$(VPP_TARGET_USES_HCP)" "YES"
include $(CLEAR_VARS)
LOCAL_MODULE := libvpphcpheaders
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/hcp/$(VPP_TARGET_DIR)
LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libvppdspheaders
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
include $(BUILD_HEADER_LIBRARY)
endif

#=============================================================================
# HVX Header Library
#=============================================================================
ifeq "$(VPP_TARGET_USES_HVX_CORE)" "YES"
include $(CLEAR_VARS)
LOCAL_MODULE := libvpphvxheaders
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/hvx/$(VPP_TARGET_DIR)
LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libvppdspheaders
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
include $(BUILD_HEADER_LIBRARY)
endif

#=============================================================================
# Immotion Header Library
#=============================================================================
ifeq "$(VPP_TARGET_USES_IMMOTION)" "YES"
include $(CLEAR_VARS)
LOCAL_MODULE := libvppimmotionheaders
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/immotion
LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libvppdspheaders
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
include $(BUILD_HEADER_LIBRARY)
endif

CUR_DIR := $(filter-out $(LOCAL_PATH),$(CUR_DIR))
LOCAL_PATH := $(lastword $(CUR_DIR))
