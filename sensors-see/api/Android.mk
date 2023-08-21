LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES :=
LOCAL_C_INCLUDES += $(shell find $(LOCAL_PATH) -type d -name 'inc' -print )

LOCAL_MODULE := libsns_api
LOCAL_CLANG := true
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES :=
LOCAL_HEADER_LIBRARIES += libnanopb_headers
LOCAL_HEADER_LIBRARIES += libqmi_encdec_headers
LOCAL_HEADER_LIBRARIES += libqmi_common_headers

LOCAL_SHARED_LIBRARIES :=
LOCAL_SHARED_LIBRARIES += libnanopb
LOCAL_SHARED_LIBRARIES += libqmi_common_so

LOCAL_SRC_FILES += \
  $(shell find $(LOCAL_PATH)/* -name '*.c' | grep  'src/.*\.c' | sed s:^$(LOCAL_PATH)/::g )

LOCAL_CFLAGS += -std=c11 -DPB_FIELD_16BIT
LOCAL_CFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-maybe-uninitialized
ifeq ($(LLVM_SENSORS_SEE),true)
LOCAL_CFLAGS += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libsns_api_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc
include $(BUILD_HEADER_LIBRARY)
