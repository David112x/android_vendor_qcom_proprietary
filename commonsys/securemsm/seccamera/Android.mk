ifneq ($(TARGET_HAS_LOW_RAM), true)
include $(call all-subdir-makefiles)
endif
