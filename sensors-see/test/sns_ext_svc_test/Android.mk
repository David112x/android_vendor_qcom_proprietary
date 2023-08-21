ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

COMMON_INCLUDES :=
COMMON_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_intermediates/proto
COMMON_INCLUDES += vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc
COMMON_INCLUDES += vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log

LOCAL_HEADER_LIBRARIES :=
LOCAL_HEADER_LIBRARIES += libqmi_encdec_headers
LOCAL_HEADER_LIBRARIES += libqmi_common_headers

LOCAL_C_INCLUDES := $(COMMON_INCLUDES)
LOCAL_C_INCLUDES += $(shell find $(LOCAL_PATH) -type d -name 'inc' -print )

LOCAL_MODULE := sns_ext_svc_test
LOCAL_CLANG := true
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libprotobuf-cpp-full
LOCAL_SHARED_LIBRARIES += libsensorslog
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libssc

LOCAL_SRC_FILES += src/location_service_v02.c
LOCAL_SRC_FILES += src/sns_ext_svc_test.cpp

LOCAL_CFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-maybe-uninitialized

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
endif
