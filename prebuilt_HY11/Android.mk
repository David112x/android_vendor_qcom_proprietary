LOCAL_PATH := $(call my-dir)
PREBUILT_DIR_PATH := $(LOCAL_PATH)

ifeq ($(call is-board-platform,kona),true)
  -include $(LOCAL_PATH)/target/product/kona/Android.mk
endif

ifeq ($(strip $(TARGET_BOARD_SUFFIX)),)
-include $(PREBUILT_DIR_PATH)/target/product/qssi/Android.mk
endif
ifeq ($(strip $(TARGET_PRODUCT)),qssi)
   ifneq ($(strip $(TARGET_PARENT_VENDOR)),msmnile)
      -include $(PREBUILT_DIR_PATH)/target/product/$(TARGET_PARENT_VENDOR)/Android.mk
   endif
endif

-include $(sort $(wildcard $(PREBUILT_DIR_PATH)/*/*/Android.mk))
