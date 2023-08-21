ifneq ($(TARGET_DISABLE_DISPLAY),true)
DISPLAY_DIR := $(call my-dir)

include $(DISPLAY_DIR)/test/Android.mk

endif #TARGET_DISABLE_DISPLAY
