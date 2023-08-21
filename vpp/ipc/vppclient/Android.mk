CUR_DIR := $(CUR_DIR) $(call my-dir)
LOCAL_PATH := $(lastword $(CUR_DIR))

#######################################################################
#
# libvppclient
#
#######################################################################
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../common.mk

LOCAL_FILES := \
	src/VppClient.cpp \
	src/HidlVppCallbacks.cpp \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/../../lib/core/inc \
	$(LOCAL_PATH)/../../lib/utils/inc \
	$(LOCAL_PATH)/../../test/utils/inc
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/inc

LOCAL_CPPFLAGS += -Wall -Werror -D_ANDROID_ -DVPP_IPC

LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_HEADER_LIBRARIES := libutils_headers
LOCAL_HEADER_LIBRARIES += display_headers display_intf_headers
LOCAL_SRC_FILES := $(LOCAL_FILES)
LOCAL_MODULE := libvppclient
LOCAL_CLANG := true
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_SHARED_LIBRARIES := \
	libhidlbase \
	libhidltransport \
	liblog \
	libhwbinder \
	libutils \
	libhardware \
	vendor.qti.hardware.vpp@1.1 \
	vendor.qti.hardware.vpp@1.2 \
	vendor.qti.hardware.vpp@1.3 \
	libvpplibrary \
	libcutils \
	libqdMetaData
LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := libvpplibrary

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_SANITIZE := $(VPP_SANITIZE)
LOCAL_SANITIZE_DIAG := $(VPP_SANITIZE_DIAG)
include $(BUILD_SHARED_LIBRARY)

CUR_DIR := $(filter-out $(LOCAL_PATH),$(CUR_DIR))
LOCAL_PATH := $(lastword $(CUR_DIR))