CUR_DIR := $(CUR_DIR) $(call my-dir)
LOCAL_PATH := $(lastword $(CUR_DIR))

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../common.mk

LOCAL_FILES := \
	src/dvpTest_tb.c \
	src/dvpTest_android.c \
	src/buf_pool.c \
	src/test_utils.c \
	src/test_mock_ip.c \
	src/test_mock_registry.c \
	src/test_gralloc.cpp \
	src/test_metadata.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/inc/ \
	$(LOCAL_PATH)/../../lib/core/inc/ \
	$(LOCAL_PATH)/../../lib/utils/inc/ \
	$(TOP)/hardware/qcom/media/mm-core/inc/

ifeq "$(VPP_TARGET_USES_GPU)" "YES"
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../lib/ip/gpu/inc/
endif

ifeq "$(VPP_TARGET_USES_HVX)" "YES"
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../lib/ip/hvx/inc/
endif

LOCAL_CFLAGS += -Wall -Werror -D_ANDROID_ -DVPP_TEST_UTILS

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_HEADER_LIBRARIES := \
	libutils_headers \
	display_headers \
	display_intf_headers \
	libvppheaders
LOCAL_SRC_FILES := $(LOCAL_FILES)
LOCAL_MODULE := libvpptestutils
LOCAL_CLANG := true
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := libcutils liblog libvpplibrary
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_SANITIZE := $(VPP_SANITIZE)
LOCAL_SANITIZE_DIAG := $(VPP_SANITIZE_DIAG)
include $(BUILD_SHARED_LIBRARY)

CUR_DIR := $(filter-out $(LOCAL_PATH),$(CUR_DIR))
LOCAL_PATH := $(lastword $(CUR_DIR))
