CUR_DIR := $(call my-dir)

ifeq ($(TARGET_USES_QMAA),true)
  ifneq ($(TARGET_USES_QMAA_OVERRIDE_VPP),true)
    TARGET_DISABLE_QTI_VPP := true
  endif # TARGET_USES_QMAA_OVERRIDE_VPP
endif # TARGET_USES_QMAA

ifeq ($(call is-board-platform-in-list, $(MSM_VIDC_TARGET_LIST)),true)
ifneq ($(strip $(TARGET_DISABLE_QTI_VPP)),true)

include $(CUR_DIR)/common.mk
include $(call all-subdir-makefiles)

endif
endif
