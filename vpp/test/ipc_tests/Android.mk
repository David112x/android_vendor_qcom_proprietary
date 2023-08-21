CUR_DIR := $(CUR_DIR) $(call my-dir)
LOCAL_PATH := $(lastword $(CUR_DIR))

#######################################################################
#
# vppserviceunittest
#
#######################################################################
include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../common.mk

LOCAL_FILES := \
	src/test_svc_main.cpp \
	src/test_ipc_utils.cpp \
	src/test_ipc_service.cpp \
	../../ipc/service/1.x/src/HidlVpp.cpp \
	../../ipc/service/1.x/src/HidlVppUtils.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../ipc/service/1.x/inc \
	$(LOCAL_PATH)/../core/inc/ \
	$(LOCAL_PATH)/../utils/inc/ \
	$(LOCAL_PATH)/../../lib/utils/inc \
	$(LOCAL_PATH)/../../lib/core/inc \

LOCAL_CPPFLAGS += -Wall -Werror -D_ANDROID_ -fexceptions

LOCAL_32_BIT_ONLY := true
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_SRC_FILES := $(LOCAL_FILES)
LOCAL_MODULE := vppserviceunittest
LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES += libcutils liblog libvpplibrary libvpptestutils
LOCAL_SHARED_LIBRARIES += \
	libhidlbase \
	libhidltransport \
	libhwbinder \
	libutils \
	libhardware \
	vendor.qti.hardware.vpp@1.1 \
	vendor.qti.hardware.vpp@1.2 \
	vendor.qti.hardware.vpp@1.3 \
	libvppclient \
	libqdMetaData

LOCAL_HEADER_LIBRARIES := display_headers display_intf_headers

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_SANITIZE := $(VPP_SANITIZE)
LOCAL_SANITIZE_DIAG := $(VPP_SANITIZE_DIAG)
include $(BUILD_EXECUTABLE)

#######################################################################
#
# vppipcunittest
#
#######################################################################
LOCAL_FILES := \
	src/test_ipc_main.cpp \
	src/test_ipc_client.cpp \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../core/inc/ \
	$(LOCAL_PATH)/../utils/inc/ \
	$(LOCAL_PATH)/../../lib/utils/inc \
	$(LOCAL_PATH)/../../lib/core/inc \

LOCAL_CPPFLAGS += -Wall -Werror -D_ANDROID_ -fexceptions

LOCAL_32_BIT_ONLY := true
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_SRC_FILES := $(LOCAL_FILES)
LOCAL_MODULE := vppipcunittest
LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES += libcutils liblog libvpplibrary libvpptestutils
LOCAL_SHARED_LIBRARIES += \
	libhidlbase \
	libhidltransport \
	libhwbinder \
	libutils \
	libhardware \
	vendor.qti.hardware.vpp@1.1 \
	vendor.qti.hardware.vpp@1.2 \
	vendor.qti.hardware.vpp@1.3 \
	libvppclient \
	libqdMetaData

LOCAL_HEADER_LIBRARIES := display_headers display_intf_headers

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_SANITIZE := $(VPP_SANITIZE)
LOCAL_SANITIZE_DIAG := $(VPP_SANITIZE_DIAG)
include $(BUILD_EXECUTABLE)

CUR_DIR := $(filter-out $(LOCAL_PATH),$(CUR_DIR))
LOCAL_PATH := $(lastword $(CUR_DIR))
