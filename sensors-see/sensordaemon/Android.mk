ifeq ($(call is-vendor-board-platform,QCOM),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(USE_SENSOR_HAL_VER),2.0)
LOCAL_CFLAGS   := -DUSE_SENSOR_HAL_VER_2_0
endif

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../sscrpcd/inc \
        $(TARGET_OUT_HEADERS)/common/inc \
        $(TARGET_OUT_HEADERS)/diag/include \
        vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log \
        vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc \
        $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_intermediates/proto

LOCAL_SRC_FILES:= \
        src/sensordaemon.cpp \
        src/aont.cpp

LOCAL_SHARED_LIBRARIES := liblog libdiag libssc libprotobuf-cpp-full libsensorslog libcutils
LOCAL_CFLAGS += -Werror -Wall -fexceptions
ifeq ($(LLVM_SENSORS_SEE),true)
LOCAL_CFLAGS += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif

LOCAL_MODULE := sensors.qti

LOCAL_PROPRIETARY_MODULE := true
LOCAL_PRELINK_MODULE := false
LOCAL_UNINSTALLABLE_MODULE :=
LOCAL_MODULE_OWNER := qti

include $(BUILD_EXECUTABLE)

endif

ifeq ($(call is-board-platform-in-list, bengal lito),true)
include $(CLEAR_VARS)
LOCAL_MODULE       := init.vendor.sensors.rc
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES    := init.vendor.sensors.rc
LOCAL_MODULE_PATH  := $(TARGET_OUT_VENDOR_ETC)/init
include $(BUILD_PREBUILT)
endif