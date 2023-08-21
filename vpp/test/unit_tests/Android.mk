CUR_DIR := $(CUR_DIR) $(call my-dir)
LOCAL_PATH := $(lastword $(CUR_DIR))

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../common.mk

LOCAL_FILES := \
	src/test_buf.c \
	src/test_core.c \
	src/test_configstore.c \
	src/test_data_queue.c \
	src/test_ip.c \
	src/test_pipeline.c \
	src/test_queue.c \
	src/test_reg.c \
	src/test_stats.c \
	src/test_tunings.c \
	src/test_uc.c \
	src/test_vpp.c \
	src/test_vpp_utils.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../core/inc/ \
	$(LOCAL_PATH)/../utils/inc/ \
	$(LOCAL_PATH)/../../lib/utils/inc \
	$(LOCAL_PATH)/../../lib/core/inc

ifeq "$(VPP_TARGET_USES_GPU)" "YES"
LOCAL_FILES += \
	src/test_ip_gpu.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../lib/ip/gpu/inc

LOCAL_HEADER_LIBRARIES +=
LOCAL_SHARED_LIBRARIES +=
endif

ifeq "$(VPP_TARGET_USES_HVX_CORE)" "YES"
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../lib/ip/hvx_core/inc

LOCAL_HEADER_LIBRARIES += libvpphvxheaders
LOCAL_SHARED_LIBRARIES += libvpphvx
endif

ifeq "$(VPP_TARGET_USES_FRC_ME)" "YES"
LOCAL_FILES += \
    src/test_ip_me.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../lib/ip/frc/inc

LOCAL_HEADER_LIBRARIES +=
LOCAL_SHARED_LIBRARIES +=
endif

ifeq "$(VPP_TARGET_USES_FRC_MC)" "YES"
LOCAL_FILES += \
    src/test_ip_mc.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../lib/ip/frc/inc

LOCAL_HEADER_LIBRARIES +=
LOCAL_SHARED_LIBRARIES +=
endif

ifeq "$(VPP_TARGET_USES_HVX)" "YES"
LOCAL_FILES += \
    src/test_ip_hvx.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../lib/ip/hvx/inc

LOCAL_HEADER_LIBRARIES += libvpphvxheaders
LOCAL_SHARED_LIBRARIES += libvpphvx
endif

ifeq "$(VPP_TARGET_USES_HCP)" "YES"
LOCAL_FILES += \
    src/test_ip_hcp.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../lib/ip/hcp/inc

LOCAL_HEADER_LIBRARIES += libvpphcpheaders
LOCAL_SHARED_LIBRARIES += libvpphcp
endif

ifeq "$(VPP_TARGET_USES_C2D)" "YES"
LOCAL_FILES += \
    src/test_ip_c2d.c

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../../lib/ip/c2d/inc

LOCAL_HEADER_LIBRARIES +=
LOCAL_SHARED_LIBRARIES +=
endif

LOCAL_CFLAGS += -Wall -Werror -D_ANDROID_
LOCAL_CFLAGS += -DVPP_UNIT_TEST=1

LOCAL_32_BIT_ONLY := true
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include
LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_SRC_FILES := $(LOCAL_FILES)
LOCAL_MODULE := vpplibraryunittest
LOCAL_CLANG := true
LOCAL_SHARED_LIBRARIES += libcutils liblog libvpplibrary libvpptestutils
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
