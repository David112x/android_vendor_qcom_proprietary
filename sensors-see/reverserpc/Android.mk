ifeq ($(call is-vendor-board-platform,QCOM),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/inc \
        $(TARGET_OUT_HEADERS)/common/inc

LOCAL_SRC_FILES:= \
        src/sns_registry_imp.c \
        src/sns_registry_skel.c

LOCAL_SHARED_LIBRARIES := libcutils liblog libutils
LOCAL_CFLAGS += -Werror -Wall -fexceptions
ifeq ($(LLVM_SENSORS_SEE),true)
LOCAL_CFLAGS += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif

LOCAL_MODULE := libsns_registry_skel
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_OWNER := qti

include $(BUILD_SHARED_LIBRARY)

endif
