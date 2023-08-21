ifeq ($(TARGET_HAS_LOW_RAM),true)
BOARD_IPA_LOW_RAM_EXCP_LIST := bengal
ifeq ($(call is-board-platform-in-list,$(BOARD_IPA_LOW_RAM_EXCP_LIST)),true)
	include $(call all-subdir-makefiles)
endif
else
ifneq ($(TARGET_HAS_LOW_RAM),true)
BOARD_PLATFORM_LIST := msm8916
BOARD_PLATFORM_LIST += msm8909
BOARD_PLATFORM_LIST += msm8917
ifneq ($(call is-board-platform-in-list,$(BOARD_PLATFORM_LIST)),true)
include $(call all-subdir-makefiles)
endif
endif

endif # QCOM_TARGET_PRODUCT
