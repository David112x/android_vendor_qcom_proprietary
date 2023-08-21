LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# ---------------------------------------------------------------------------------
#               Common definitons
# ---------------------------------------------------------------------------------
include $(BUILD_COPY_HEADERS)

libvpp-omx-def += -g -O3
libvpp-omx-def += -Werror
#libvpp-omx-def += -Wno-parentheses

PER_EXTRADATA_TARGETS := msmnile sdmshrike $(MSMSTEPPE) atoll

ifneq (,$(strip $(filter $(PER_EXTRADATA_TARGETS),$(TARGET_BOARD_PLATFORM))))
libvpp-omx-def += -DVDEC_REQUIRES_PER_EXTRADATA_ENABLEMENT
endif

include $(CLEAR_VARS)

# Common Includes
libvpp-omx-inc          := $(LOCAL_PATH)/inc
libvpp-omx-inc          += $(TOP)/hardware/qcom/media/mm-video-v4l2/vidc/common/inc
libvpp-omx-inc          += $(TOP)/hardware/qcom/media/mm-core/inc
libvpp-omx-inc          += $(TOP)/hardware/qcom/media/mm-core/src/common
libvpp-omx-inc          += $(TOP)/vendor/qcom/proprietary/vpp/lib/core/inc/
libvpp-omx-inc          += $(TOP)/frameworks/native/include/media/openmax
libvpp-omx-inc          += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

libvpp-omx-inc += $(TOP)/hardware/qcom/media/libstagefrighthw

# Common Dependencies
libvpp-omx-add-dep := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

# ---------------------------------------------------------------------------------
#           Make the Shared library (libOmxVpp)
# ---------------------------------------------------------------------------------

include $(CLEAR_VARS)

LOCAL_MODULE                    := libOmxVpp
LOCAL_MODULE_TAGS               := optional
LOCAL_CFLAGS                    := $(libvpp-omx-def)

ifdef LLVM_REPORT_DIR
LOCAL_CFLAGS      += --compile-and-analyze $(LLVM_REPORT_DIR)
LOCAL_CPPFLAGS    += --compile-and-analyze $(LLVM_REPORT_DIR)
endif

LOCAL_C_INCLUDES                += $(libvpp-omx-inc)
LOCAL_ADDITIONAL_DEPENDENCIES   := $(libvpp-omx-add-dep)

LOCAL_PRELINK_MODULE    := false
LOCAL_SHARED_LIBRARIES  := liblog libutils libbinder libcutils libdl libOmxCore
LOCAL_SHARED_LIBRARIES  += libvppclient
LOCAL_SHARED_LIBRARIES  += vendor.qti.hardware.vpp@1.1
LOCAL_SHARED_LIBRARIES  += vendor.qti.hardware.vpp@1.2
LOCAL_SHARED_LIBRARIES  += vendor.qti.hardware.capabilityconfigstore@1.0
LOCAL_SHARED_LIBRARIES  += libhidlbase libhidltransport libhwbinder

LOCAL_STATIC_LIBRARIES  := libOmxVidcCommon
LOCAL_HEADER_LIBRARIES  += display_headers display_intf_headers libmedia_headers

LOCAL_SRC_FILES         := src/vpp_omx_component.cpp
LOCAL_SRC_FILES         += src/vpp_buffer_manager.cpp

LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_SANITIZE := integer_overflow
#LOCAL_SANITIZE_DIAG := integer_overflow
include $(BUILD_SHARED_LIBRARY)

# ---------------------------------------------------------------------------------
#                END
# ---------------------------------------------------------------------------------
