LOCAL_PATH := $(call my-dir)
QCRIL_DIR := ${LOCAL_PATH}/../..
#
include $(CLEAR_VARS)

LOCAL_CFLAGS               += -Wall -Werror $(qcril_cflags)
LOCAL_CXXFLAGS             += -std=c++17 $(qcril_cppflags)
LOCAL_CPPFLAGS             += -std=c++17 $(qcril_cppflags)
LOCAL_LDFLAGS              += $(qcril_ldflags)

ifneq ($(qcril_sanitize_diag),)
LOCAL_SANITIZE_DIAG := $(qcril_sanitize_diag)
endif

ifeq ($(QCRIL_BUILD_WITH_ASAN),true)
LOCAL_SANITIZE             += $(qcril_sanitize)
endif
LOCAL_SRC_FILES            += $(call all-cpp-files-under, src)

LOCAL_MODULE               := qcrilRadioConfigModule
LOCAL_MODULE_OWNER         := qti
LOCAL_PROPRIETARY_MODULE   := true
LOCAL_MODULE_TAGS          := optional
LOCAL_SHARED_LIBRARIES     += android.hardware.radio.config@1.0
LOCAL_SHARED_LIBRARIES     += android.hardware.radio.config@1.1
LOCAL_SHARED_LIBRARIES     += android.hardware.radio.config@1.2
LOCAL_SHARED_LIBRARIES     += android.hardware.radio@1.0
LOCAL_SHARED_LIBRARIES     += qtimutex
LOCAL_HEADER_LIBRARIES     += libril-qti-hal-qmi-headers
LOCAL_HEADER_LIBRARIES     += qtimutex-headers

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
## Build header library
LOCAL_MODULE               := radioconfig-headers
LOCAL_VENDOR_MODULE        := true
LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)/src
include $(BUILD_HEADER_LIBRARY)
