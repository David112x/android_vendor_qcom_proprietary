LOCAL_PATH := $(call my-dir)

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
# Include the prebuilt sns_low_lat_stream_skel shared library
ifneq ($(USE_SENSOR_HAL_VER),2.0)
include $(CLEAR_VARS)
LOCAL_SRC_FILES     := prebuilt/lib/q6/libsns_low_lat_stream_skel.so
LOCAL_MODULE        := libsns_low_lat_stream_skel
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_OWNER  := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH   := $(TARGET_OUT_VENDOR)/lib/rfsa/adsp
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_SRC_FILES     := prebuilt/lib/q6/libsns_low_lat_stream_skel.so
LOCAL_MODULE        := libsns_low_lat_stream_skel_system
LOCAL_MODULE_CLASS  := ETC
LOCAL_MODULE_PATH   := $(TARGET_OUT)/lib/rfsa/adsp
LOCAL_INSTALLED_MODULE_STEM := libsns_low_lat_stream_skel.so
include $(BUILD_PREBUILT)

ifneq ($(USE_SENSOR_HAL_VER),2.0)
# Compile the sns_low_lat_stream_stub shared library
include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sns_low_lat_stream_stub.c
LOCAL_MODULE    := libsns_low_lat_stream_stub
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../sns_fastRPC_utils/inc
LOCAL_CFLAGS := -Werror -Wall -Wno-unused-parameter -fexceptions
LOCAL_SHARED_LIBRARIES += libsns_fastRPC_util libdl liblog libc libutils libcutils
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER  := qti
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sns_low_lat_stream_stub.c
LOCAL_MODULE    := libsns_low_lat_stream_stub_system
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../sns_fastRPC_utils/inc
LOCAL_CFLAGS := -Werror -Wall -Wno-unused-parameter -fexceptions
LOCAL_SHARED_LIBRARIES += libsns_fastRPC_util_system libdl liblog libc libprotobuf-cpp-full
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_INSTALLED_MODULE_STEM := libsns_low_lat_stream_stub.so
include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
commonSources  :=
commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/qmi/inc
commonIncludes += $(TARGET_OUT_HEADERS)/sensors/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(TARGET_OUT_HEADERS)/qmi-framework/inc
commonIncludes += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_intermediates/proto

LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../sns_fastRPC_utils/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../adsprpc/inc/
LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE)

LOCAL_MODULE    := libsensor_low_lat
LOCAL_SRC_FILES += jni/src/sensor_low_lat.cpp

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libdiag
LOCAL_SHARED_LIBRARIES += libssc
LOCAL_SHARED_LIBRARIES += libprotobuf-cpp-full
LOCAL_SHARED_LIBRARIES += libdl libc
LOCAL_SHARED_LIBRARIES += libsns_low_lat_stream_stub
LOCAL_SHARED_LIBRARIES += libsns_fastRPC_util
ifeq ($(call is-platform-sdk-version-at-least,29),true)
LOCAL_SANITIZE := integer_overflow
endif
#LOCAL_SANITIZE_DIAG := integer_overflow
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
LOCAL_PRELINK_MODULE:=false

LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
commonSources  :=
commonIncludes := $(TARGET_OUT_HEADERS)/common/inc
commonIncludes += $(TARGET_OUT_HEADERS)/qmi/inc
commonIncludes += $(TARGET_OUT_HEADERS)/sensors/inc
commonIncludes += $(TARGET_OUT_HEADERS)/diag/include
commonIncludes += $(TARGET_OUT_HEADERS)/qmi-framework/inc
commonIncludes += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_system_intermediates/proto

LOCAL_C_INCLUDES := $(commonIncludes)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../sns_fastRPC_utils/inc
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../adsprpc/inc/
LOCAL_C_INCLUDES += $(JNI_H_INCLUDE)

LOCAL_MODULE    := libsensor_low_lat_system
LOCAL_SRC_FILES += jni/src/sensor_low_lat.cpp

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libdiag_system
LOCAL_SHARED_LIBRARIES += libprotobuf-cpp-full
LOCAL_SHARED_LIBRARIES += libdl libc
LOCAL_SHARED_LIBRARIES += libssc_system
LOCAL_SHARED_LIBRARIES += libsns_low_lat_stream_stub_system
LOCAL_SHARED_LIBRARIES += libsns_fastRPC_util_system
ifeq ($(call is-platform-sdk-version-at-least,29),true)
LOCAL_SANITIZE := integer_overflow
endif
#LOCAL_SANITIZE_DIAG := integer_overflow
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
LOCAL_PRELINK_MODULE:=false
LOCAL_INSTALLED_MODULE_STEM := libsensor_low_lat.so

include $(BUILD_SHARED_LIBRARY)
endif
