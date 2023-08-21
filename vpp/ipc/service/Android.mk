CUR_DIR := $(CUR_DIR) $(call my-dir)
LOCAL_PATH := $(lastword $(CUR_DIR))

#######################################################################
#
# libvppservice
#
#######################################################################
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../common.mk

LOCAL_MODULE := vppservice

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_CPPFLAGS := -Wall -Werror -Wextra -D_ANDROID_ -DVPP_IPC
LOCAL_SRC_FILES := \
	VppServiceMain.cpp \
	1.x/src/HidlVpp.cpp \
	1.x/src/HidlVppService.cpp \
	1.x/src/HidlVppUtils.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/1.x/inc \
	$(LOCAL_PATH)/../../lib/core/inc \
	$(LOCAL_PATH)/../../lib/utils/inc \
	$(LOCAL_PATH)/../../test/utils/inc \

LOCAL_HEADER_LIBRARIES := libutils_headers
LOCAL_HEADER_LIBRARIES += display_headers display_intf_headers

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
	libcutils \
	libvpplibrary \
	libqdMetaData

LOCAL_SANITIZE := $(VPP_SANITIZE)
LOCAL_SANITIZE_DIAG := $(VPP_SANITIZE_DIAG)

LOCAL_INIT_RC := vppservice.rc

include $(BUILD_EXECUTABLE)

CUR_DIR := $(filter-out $(LOCAL_PATH),$(CUR_DIR))
LOCAL_PATH := $(lastword $(CUR_DIR))
