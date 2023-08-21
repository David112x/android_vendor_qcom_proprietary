LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := sns_qsocket_test
LOCAL_CLANG := true
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true

LOCAL_HEADER_LIBRARIES :=
LOCAL_HEADER_LIBRARIES += libnanopb_headers
LOCAL_HEADER_LIBRARIES += libsns_api_headers
LOCAL_HEADER_LIBRARIES += libqmi_cci_headers
LOCAL_HEADER_LIBRARIES += libqmi_encdec_headers
LOCAL_HEADER_LIBRARIES += libqmi_common_headers
LOCAL_HEADER_LIBRARIES += libqsocket_headers

LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libnanopb
LOCAL_SHARED_LIBRARIES += libsns_api
LOCAL_SHARED_LIBRARIES += libqsocket

LOCAL_SRC_FILES += \
  $(shell find $(LOCAL_PATH)/* -name '*.c' | grep  'src/.*\.c' | sed s:^$(LOCAL_PATH)/::g )

# Whether to use QSocket (true) or QMI (false) implementation
SNS_USE_QSOCKET = true

ifeq ($(SNS_USE_QSOCKET),true)
  LOCAL_SRC_FILES := $(filter-out src/sns_qmi_client.c , $(LOCAL_SRC_FILES))
else
  LOCAL_SRC_FILES := $(filter-out src/sns_qsocket_client.c , $(LOCAL_SRC_FILES))
  LOCAL_SHARED_LIBRARIES += libqmi_common_so
  LOCAL_SHARED_LIBRARIES += libqmi_cci
  LOCAL_SHARED_LIBRARIES += libqmi_encdec
  LOCAL_SHARED_LIBRARIES += libqmi_csi
endif

LOCAL_C_INCLUDES :=
LOCAL_C_INCLUDES += $(shell find $(LOCAL_PATH) -type d -name 'inc' -print )

LOCAL_CFLAGS += -std=c11 -DSSC_TARGET_X86 -DPB_FIELD_16BIT
LOCAL_CFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-maybe-uninitialized
ifeq ($(LLVM_SENSORS_SEE),true)
  LOCAL_CFLAGS += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
