VIDC_STUB_HAL := false
ifeq ($(TARGET_USES_QMAA),true)
  ifneq ($(TARGET_USES_QMAA_OVERRIDE_VIDEO),true)
    VIDC_STUB_HAL := true
  endif #TARGET_USES_QMAA_OVERRIDE_VIDEO
endif #TARGET_USES_QMAA

ifeq ($(VIDC_STUB_HAL),false)
LOCAL_PATH := $(call my-dir)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif #VIDC_STUB_HAL
