ifeq ($(BOARD_HAS_QCOM_WLAN), true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES :=
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-int-to-pointer-cast
LOCAL_CFLAGS += -Wno-maybe-uninitialized -Wno-parentheses

LOCAL_MODULE := cfrtool
ifeq ($(PRODUCT_VENDOR_MOVE_ENABLED), true)
LOCAL_PROPRIETARY_MODULE := true
endif
LOCAL_CLANG := true
LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES :=

LOCAL_SHARED_LIBRARIES :=

LOCAL_SRC_FILES :=
LOCAL_SRC_FILES += cfrtool.c

LOCAL_LDLIBS += -Lpthread
LOCAL_SANITIZE := integer_overflow
include $(BUILD_EXECUTABLE)

endif # Global WLAN enable check
