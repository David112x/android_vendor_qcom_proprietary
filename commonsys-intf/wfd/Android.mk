ifeq ($(call is-vendor-board-platform,QCOM),true)
WFD_DISABLE_PLATFORM_LIST := msm8610 mpq8092 msm_bronze msm8909 msm8952

#Disable for selected 32-bit targets
ifeq ($(call is-board-platform,bengal),true)
ifeq ($(TARGET_BOARD_SUFFIX),_32)
WFD_DISABLE_PLATFORM_LIST += bengal
endif
endif

ifneq ($(call is-board-platform-in-list,$(WFD_DISABLE_PLATFORM_LIST)),true)
ifneq ($(TARGET_HAS_LOW_RAM), true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libwfd_headers
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/./utils/inc
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/./wfd_headers
include $(BUILD_HEADER_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libwfd_proprietary_headers
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/./utils/inc
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/./wfd_headers
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_HEADER_LIBRARY)

include $(call all-makefiles-under, $(LOCAL_PATH))
endif
endif

endif # TARGET_USES_WFD
