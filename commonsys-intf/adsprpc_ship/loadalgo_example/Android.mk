LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(TARGET_OUT_HEADERS)/fastrpc/inc

LOCAL_SHARED_LIBRARIES := \
        libc \
        libcdsprpc
LOCAL_MODULE := libloadalgo_stub
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := loadalgo_stub.c
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TARGET_ARCHS:= arm64
LOCAL_MULTILIB := 64
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
                    $(LOCAL_PATH) \
                    $(TARGET_OUT_HEADERS)/fastrpc/inc \
		    vendor/qcom/proprietary/securemsm/QSEEComAPI \
                    $(TARGET_OUT_HEADERS)/common/inc

LOCAL_SHARED_LIBRARIES := \
        libc \
	libutils \
        libQSEEComAPI \
        libdl \
	libion \
	libloadalgo_stub \
        liblog

LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk \
                                 $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_MODULE := loadalgo
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := secure_memory.c \
		   loadalgo.c
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := $(QSEECOM_CFLAGS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TARGET_ARCHS:= arm64
LOCAL_MULTILIB := 64
include $(BUILD_EXECUTABLE)

