LOCAL_PATH := $(call my-dir)

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)


# Include the prebuilt sns_low_lat_stream_skel shared library
include $(CLEAR_VARS)
LOCAL_SRC_FILES     := prebuild/lib/q6/libsns_device_mode_skel.so
LOCAL_MODULE        := libsns_device_mode_skel
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_OWNER  := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH   := $(TARGET_OUT_VENDOR)/lib/rfsa/adsp
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES     := prebuild/lib/q6/libsns_device_mode_skel.so
LOCAL_MODULE        := libsns_device_mode_skel_system
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_PATH   := $(TARGET_OUT)/lib/rfsa/adsp
LOCAL_INSTALLED_MODULE_STEM := libsns_device_mode_skel.so
include $(BUILD_PREBUILT)

# Compile the sns_device_mode_stub shared library
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sns_device_mode_stub.c
LOCAL_MODULE    := libsns_device_mode_stub
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../sns_fastRPC_utils/inc/
LOCAL_CFLAGS := -Werror -Wall -Wno-unused-parameter -fexceptions
LOCAL_SHARED_LIBRARIES += libsns_fastRPC_util libdl liblog libc libutils libcutils

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER  := qti
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sns_device_mode_stub.c
LOCAL_MODULE    := libsns_device_mode_stub_system
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../sns_fastRPC_utils/inc/
LOCAL_CFLAGS := -Werror -Wall -Wno-unused-parameter -fexceptions
LOCAL_SHARED_LIBRARIES += libsns_fastRPC_util_system libdl liblog libc libutils libcutils

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_INSTALLED_MODULE_STEM := libsns_device_mode_stub.so
include $(BUILD_SHARED_LIBRARY)


endif
