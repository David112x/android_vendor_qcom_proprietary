VIDEO_DIR := $(call my-dir)
LOCAL_PATH := $(VIDEO_DIR)
include $(CLEAR_VARS)

ifeq ($(TARGET_USES_QMAA),true)
  ifneq ($(TARGET_USES_QMAA_OVERRIDE_VIDEO),true)
    TARGET_DISABLE_QTI_VPP := true
  endif #TARGET_USES_QMAA_OVERRIDE_VIDEO

  ifneq ($(TARGET_USES_QMAA_OVERRIDE_VPP),true)
    TARGET_DISABLE_QTI_VPP := true
  endif # TARGET_USES_QMAA_OVERRIDE_VPP
endif #TARGET_USES_QMAA

ifeq ($(call is-board-platform-in-list, $(MSM_VIDC_TARGET_LIST)),true)
ifneq ($(strip $(TARGET_DISABLE_QTI_VPP)),true)
include $(VIDEO_DIR)/omx_vpp/Android.mk
endif
endif
