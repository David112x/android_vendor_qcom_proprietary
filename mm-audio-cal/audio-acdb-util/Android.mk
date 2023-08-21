ifneq ($(AUDIO_USE_STUB_HAL), true)
AUD_ACDB_UTIL_ROOT := $(call my-dir)
include $(call all-subdir-makefiles)
endif # AUDIO_USE_STUB_HAL
