ifeq ($(call is-vendor-board-platform,QCOM),true)
ifneq (,$(filter true, $(TARGET_FWK_SUPPORTS_FULL_VALUEADDS) $(TARGET_BOARD_AUTO)))
WFD_DISABLE_PLATFORM_LIST := msm8610 mpq8092 msm_bronze msm8909 msm8952

ifneq ($(call is-board-platform-in-list,$(WFD_DISABLE_PLATFORM_LIST)),true)
ifneq ($(TARGET_HAS_LOW_RAM), true)
LOCAL_PATH := $(call my-dir)

include $(call all-makefiles-under, $(LOCAL_PATH))
endif
endif
endif # TARGET_FWK_SUPPORTS_FULL_VALUEADDS, AUTO TARGETS
endif # TARGET_USES_WFD
