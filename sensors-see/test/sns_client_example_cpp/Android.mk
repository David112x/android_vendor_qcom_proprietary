LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
COMMON_INCLUDES :=
COMMON_INCLUDES += $(TARGET_OUT_HEADERS)/qmi-framework/inc
COMMON_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_intermediates/proto

LOCAL_C_INCLUDES := $(COMMON_INCLUDES)
LOCAL_C_INCLUDES += $(shell find $(LOCAL_PATH) -type d -name 'inc' -print )

LOCAL_MODULE := sns_client_example_cpp
LOCAL_CLANG := true
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libprotobuf-cpp-full
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libsnsapi
LOCAL_SHARED_LIBRARIES += libqmi_encdec
LOCAL_SHARED_LIBRARIES += libqmi_cci

LOCAL_SRC_FILES += /src/sns_client_example.cpp
LOCAL_SRC_FILES += /src/ssc_connection_reference.cpp
LOCAL_SRC_FILES += /src/ssc_suid_util.cpp

LOCAL_CFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-maybe-uninitialized -fexceptions

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
