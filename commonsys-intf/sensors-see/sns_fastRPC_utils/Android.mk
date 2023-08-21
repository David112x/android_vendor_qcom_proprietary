LOCAL_PATH := $(call my-dir)

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)

# Compile the sns_fastRPC_util shared library
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sns_fastRPC_utils.cpp
LOCAL_SRC_FILES += src/rpcmem_android.c
LOCAL_MODULE    := libsns_fastRPC_util
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_CFLAGS := -Wall -Wno-unused-parameter -fexceptions
LOCAL_SHARED_LIBRARIES += libdl liblog libc libutils libcutils

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER  := qti
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sns_fastRPC_utils.cpp
LOCAL_SRC_FILES += src/rpcmem_android.c
LOCAL_MODULE    := libsns_fastRPC_util_system
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_CFLAGS := -Wall -Wno-unused-parameter -fexceptions
LOCAL_SHARED_LIBRARIES += libdl liblog libc libprotobuf-cpp-full

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_INSTALLED_MODULE_STEM := libsns_fastRPC_util.so
include $(BUILD_SHARED_LIBRARY)

endif