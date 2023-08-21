ifeq ($(USE_SENSOR_HAL_VER),2.0)
LOCAL_PATH := $(call my-dir)

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
# Include the prebuilt sns_low_lat_stream_skel shared library
include $(CLEAR_VARS)
LOCAL_SRC_FILES     := prebuilt/lib/q6/libsns_low_lat_stream_skel.so
LOCAL_MODULE        := libsns_low_lat_stream_skel
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_OWNER  := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH   := $(TARGET_OUT_VENDOR)/lib/rfsa/adsp
include $(BUILD_PREBUILT)


# Compile the sns_low_lat_stream_stub shared library
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sns_low_lat_stream_stub.c
LOCAL_MODULE    := libsns_low_lat_stream_stub
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../reverserpc/inc
#LOCAL_CFLAGS := -Werror -Wall -Wno-unused-parameter -fexceptions
LOCAL_CFLAGS := -Werror -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES += libdl liblog libc libutils libcutils
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER  := qti
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)

#TARGET_SUPPORTS_WEARABLES
endif
#USE_SENSOR_HAL_VER
endif
