ifeq ($(call is-vendor-board-platform,QCOM),true)
LOCAL_PATH:= $(call my-dir)

ifneq ($(call is-board-platform-in-list,$(TRINKET) sdm710 $(MSMSTEPPE) qcs605), true)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/inc \
        $(TARGET_OUT_HEADERS)/common/inc \
        $(TARGET_OUT_INTERMEDIATES)/include/fastrpc/inc

LOCAL_SRC_FILES:= \
        src/sscrpcd.cpp \

ifneq ($(call is-board-platform-in-list, lito bengal atoll),true)
LOCAL_CFLAGS := -DADSP_DEFAULT_LISTENER_NAME=\"libssc_default_listener.so\"
endif
LOCAL_SHARED_LIBRARIES := liblog libdl
LOCAL_CFLAGS += -Werror -Wall -fexceptions
ifeq ($(LLVM_SENSORS_SEE),true)
LOCAL_CFLAGS += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif

LOCAL_MODULE := sscrpcd

LOCAL_PROPRIETARY_MODULE := true
LOCAL_PRELINK_MODULE := false
LOCAL_UNINSTALLABLE_MODULE :=
LOCAL_MODULE_OWNER := qti

include $(BUILD_EXECUTABLE)

endif
endif
