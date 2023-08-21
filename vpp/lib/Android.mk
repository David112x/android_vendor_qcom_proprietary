CUR_DIR := $(CUR_DIR) $(call my-dir)
LOCAL_PATH := $(lastword $(CUR_DIR))

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../common.mk

#=============================================================================
# Core Defines
#=============================================================================
LOCAL_FILES := \
	utils/src/vpp_queue.c \
	utils/src/vpp_data_queue.c \
	utils/src/vpp_utils.c \
	utils/src/vpp_ion.c \
	utils/src/vpp_tunings.c \
	core/src/vpp.c \
	core/src/vpp_ip.c \
	core/src/vpp_buf.c \
	core/src/vpp_uc.c \
	core/src/vpp_stats.c \
	core/src/vpp_callback.c \
	core/src/vpp_pipeline.c

LOCAL_CORE_INCLUDES := \
	$(LOCAL_PATH)/../inc \
	$(LOCAL_PATH)/core/inc \
	$(LOCAL_PATH)/utils/inc \
	$(TOP)/system/core/libion/include \
	$(TOP)/system/core/libion/kernel-headers \
	$(TOP)/hardware/qcom/media/mm-core/inc

LOCAL_CORE_HEADER_LIBS := \
	libutils_headers \
	display_headers \
	display_intf_headers \
	libvppheaders

LOCAL_SHARED_LIBRARIES :=

#=============================================================================
# Android Specific
#=============================================================================
LOCAL_FILES += \
	core/src/vpp_configstore.cpp

LOCAL_CORE_INCLUDES +=

LOCAL_CORE_HEADER_LIBS +=

LOCAL_SHARED_LIBRARIES += \
	libhidlbase \
	libutils \
	vendor.qti.hardware.capabilityconfigstore@1.0

#=============================================================================
# GPU Defines
#=============================================================================
LOCAL_GPU_FILES := \
	ip/gpu/src/vpp_ip_gpu.c

LOCAL_GPU_INCLUDES := \
	$(LOCAL_PATH)/ip/gpu/inc \
	$(TARGET_OUT_HEADERS)/vpp_library-noship

LOCAL_GPU_HEADER_LIBS :=

LOCAL_GPU_SHARED_LIBS := \
	libmmsw_detail_enhancement \
	libmmsw_opencl \
	libmmsw_platform \
	libmmsw_math

#=============================================================================
# HVX Core Defines
#=============================================================================
LOCAL_HVX_CORE_FILES := \
	ip/hvx_core/src/vpp_ip_hvx_core.c \
	ip/hvx_core/src/vpp_ip_hvx_debug.c

LOCAL_HVX_CORE_INCLUDES := \
	$(LOCAL_PATH)/ip/hvx_core/inc

LOCAL_HVX_CORE_HEADER_LIBS := \
	libvpphvxheaders

LOCAL_HVX_CORE_SHARED_LIBS := \
	libvpphvx \
	$(DSP_RPC_LIB)

#=============================================================================
# HVX Defines
#=============================================================================
LOCAL_HVX_FILES := \
	ip/hvx/src/vpp_ip_hvx.c

LOCAL_HVX_INCLUDES := \
	$(LOCAL_PATH)/ip/hvx/inc

LOCAL_HVX_HEADER_LIBS := \
	libvpphvxheaders

LOCAL_HVX_SHARED_LIBS := \
	libvpphvx \
	$(DSP_RPC_LIB)

#=============================================================================
# FRC Core Defines
#=============================================================================
LOCAL_FRC_CORE_FILES := \
	ip/frc/src/vpp_ip_frc_core.c

LOCAL_FRC_CORE_INCLUDES := \
	$(LOCAL_PATH)/ip/frc/inc

LOCAL_FRC_CORE_HEADER_LIBS :=

LOCAL_FRC_CORE_SHARED_LIBS :=

#=============================================================================
# FRC MC Defines
#=============================================================================
LOCAL_FRC_MC_FILES := \
	ip/frc/src/vpp_ip_frc_mc.c

LOCAL_FRC_MC_INCLUDES := \
	$(LOCAL_PATH)/ip/frc/inc

LOCAL_FRC_HEADER_LIBS := \
	libvpphvxheaders

LOCAL_FRC_SHARED_LIBS := \
	$(DSP_RPC_LIB)

#=============================================================================
# FRC ME Defines
#=============================================================================
LOCAL_FRC_ME_FILES := \
	ip/frc/src/vpp_ip_frc_me.c \
	ip/frc/src/vpp_ip_frc_me_tme.c

LOCAL_FRC_ME_INCLUDES := \
	$(LOCAL_PATH)/ip/frc/inc

LOCAL_FRC_ME_HEADER_LIBS :=

LOCAL_FRC_ME_SHARED_LIBS :=

#=============================================================================
# HCP Defines
#=============================================================================
LOCAL_HCP_FILES := \
	ip/hcp/src/vpp_ip_hcp_dbg.c \
	ip/hcp/src/vpp_ip_hcp_hfi.c \
	ip/hcp/src/vpp_ip_hcp.c \
	ip/hcp/src/vpp_ip_hcp_tunings.c

LOCAL_HCP_INCLUDES := \
	$(LOCAL_PATH)/ip/hcp/inc

LOCAL_HCP_HEADER_LIBS := \
	libvpphcpheaders

LOCAL_HCP_SHARED_LIBS := \
	libvpphcp \
	$(DSP_RPC_LIB)

#=============================================================================
# C2D Defines
#=============================================================================
LOCAL_C2D_FILES := \
	ip/c2d/src/vpp_ip_c2d.c

LOCAL_C2D_INCLUDES := \
	$(LOCAL_PATH)/ip/c2d/inc \
	$(TARGET_OUT_HEADERS)/adreno

LOCAL_C2D_HEADER_LIBS :=

LOCAL_C2D_SHARED_LIBS := \
	libC2D2

#=============================================================================
# msm8937 target
#=============================================================================
ifeq ($(LOCAL_BOARD_PLATFORM),msm8937)
LOCAL_PLATFORM_FILES := \
	chip/msm8937/vpp_reg_8937.c

#=============================================================================
# msm8952 target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),msm8952)
LOCAL_PLATFORM_FILES := \
	chip/msm8956/vpp_reg_8956.c

#=============================================================================
# msm8953 target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),msm8953)
LOCAL_PLATFORM_FILES := \
	chip/msm8953/vpp_reg_8953.c

#=============================================================================
# msm8996 target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),msm8996)
LOCAL_PLATFORM_FILES := \
	chip/msm8996/vpp_reg_8996.c

#=============================================================================
# msm8998 target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),msm8998)
LOCAL_PLATFORM_FILES := \
	chip/msm8998/vpp_reg_msm8998.c

#=============================================================================
# sdm660 target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),sdm660)
LOCAL_PLATFORM_FILES := \
	chip/sdm660/vpp_reg_sdm660.c

#=============================================================================
# sdm670 target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),sdm670)
LOCAL_PLATFORM_FILES := \
	chip/sdm670/vpp_reg_sdm670.c

#=============================================================================
# sdm845 target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),sdm845)
LOCAL_PLATFORM_FILES := \
	chip/sdm845/vpp_reg_sdm845.c

#=============================================================================
# msmnile target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),msmnile)
LOCAL_PLATFORM_FILES := \
	chip/msmnile/vpp_reg_msmnile.c

#=============================================================================
# sdmshrike target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),sdmshrike)
LOCAL_PLATFORM_FILES := \
	chip/sdmshrike/vpp_reg_sdmshrike.c

#=============================================================================
# msmsteppe target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),$(MSMSTEPPE))
LOCAL_PLATFORM_FILES := \
	chip/msmsteppe/vpp_reg_msmsteppe.c

#=============================================================================
# kona target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),kona)
LOCAL_PLATFORM_FILES := \
	chip/kona/vpp_reg_kona.c

#=============================================================================
# lito target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),lito)
LOCAL_PLATFORM_FILES := \
	chip/lito/vpp_reg_lito.c

#=============================================================================
# atoll target
#=============================================================================
else ifeq ($(LOCAL_BOARD_PLATFORM),atoll)
LOCAL_PLATFORM_FILES := \
	chip/atoll/vpp_reg_atoll.c

#=============================================================================
# others
#=============================================================================
else
LOCAL_PLATFORM_FILES := \
	chip/default/vpp_reg_default.c
endif

ifeq "$(VPP_TARGET_USES_GPU)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_GPU_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_GPU_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_GPU_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_GPU_SHARED_LIBS)
endif

ifeq "$(VPP_TARGET_USES_HVX_CORE)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_HVX_CORE_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_HVX_CORE_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_HVX_CORE_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_HVX_CORE_SHARED_LIBS)
endif

ifeq "$(VPP_TARGET_USES_HVX)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_HVX_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_HVX_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_HVX_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_HVX_SHARED_LIBS)
endif

ifeq "$(VPP_TARGET_USES_FRC_CORE)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_FRC_CORE_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_FRC_CORE_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_FRC_CORE_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_FRC_CORE_SHARED_LIBS)
endif

ifeq "$(VPP_TARGET_USES_FRC_ME)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_FRC_ME_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_FRC_ME_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_FRC_ME_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_FRC_ME_SHARED_LIBS)
endif

ifeq "$(VPP_TARGET_USES_FRC_MC)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_FRC_MC_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_FRC_MC_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_FRC_MC_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_FRC_MC_SHARED_LIBS)
endif

ifeq "$(VPP_TARGET_USES_HCP)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_HCP_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_HCP_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_HCP_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_HCP_SHARED_LIBS)
endif

ifeq "$(VPP_TARGET_USES_C2D)" "YES"
LOCAL_PLATFORM_FILES += $(LOCAL_C2D_FILES)
LOCAL_PLATFORM_INCLUDES += $(LOCAL_C2D_INCLUDES)
LOCAL_PLATFORM_HEADER_LIBRARIES += $(LOCAL_C2D_HEADER_LIBS)
LOCAL_PLATFORM_SHARED_LIBRARIES += $(LOCAL_C2D_SHARED_LIBS)
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_CORE_INCLUDES) \
	$(LOCAL_PLATFORM_INCLUDES) \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_HEADER_LIBRARIES := \
	$(LOCAL_CORE_HEADER_LIBS) \
	$(LOCAL_PLATFORM_HEADER_LIBRARIES)
LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := libvppheaders
LOCAL_CFLAGS += -Wall -Werror
LOCAL_CFLAGS += -DVPP_LIBRARY -D_ANDROID_
LOCAL_SRC_FILES := $(LOCAL_FILES) $(LOCAL_PLATFORM_FILES)
LOCAL_MODULE := libvpplibrary
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES += libdl libcutils liblog libqdMetaData libion
LOCAL_SHARED_LIBRARIES += $(LOCAL_PLATFORM_SHARED_LIBRARIES)
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_SANITIZE := $(VPP_SANITIZE)
LOCAL_SANITIZE_DIAG := $(VPP_SANITIZE_DIAG)
include $(BUILD_SHARED_LIBRARY)

CUR_DIR := $(filter-out $(LOCAL_PATH),$(CUR_DIR))
LOCAL_PATH := $(lastword $(CUR_DIR))
