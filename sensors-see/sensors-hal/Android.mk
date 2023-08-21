ifneq ($(USE_SENSOR_HAL_VER),2.0)
ifeq ($(call is-board-platform-in-list,sdm845 msmnile sdm710 qcs605 $(MSMSTEPPE) kona lito atoll),true)
TARGET_SUPPORT_DIRECT_REPORT :=true
endif

ifeq ($(call is-board-platform-in-list, sdm845 sdm710 qcs605 $(MSMSTEPPE)),true)
NO_SNS_FIRMWARE_SUPPORT_FOR_PROPERTY_READING := true
endif

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

c_sources = $(call all-cpp-files-under, .)

#$(info sources = $(c_sources))

# Include paths
c_includes =
c_includes += $(LOCAL_PATH)/framework
ifeq ($(TARGET_SUPPORT_DIRECT_REPORT), true)
    c_includes += vendor/qcom/proprietary/commonsys-intf/sensors-see/sns_low_lat/inc
    c_includes += vendor/qcom/proprietary/commonsys-intf/sensors-see/sns_fastRPC_utils/inc
endif
# TODO: try to use automatic export_includes from libssc.so, to remove following
# dependencies
c_includes += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_intermediates/proto
c_includes += vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc
c_includes += vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log
c_includes += vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-diag-log/inc

# shared libraries to link
shared_libs =
shared_libs += libssc
shared_libs += liblog
shared_libs += libcutils
shared_libs += libsensorslog
shared_libs += libprotobuf-cpp-full
shared_libs += libutils
shared_libs += libsnsdiaglog
shared_libs += libhardware
shared_libs += libdl
ifeq ($(TARGET_SUPPORT_DIRECT_REPORT), true)
    shared_libs += libsns_low_lat_stream_stub
    shared_libs += libsns_fastRPC_util
    ifeq ($(call is-board-platform-in-list, sdm845 msmnile kona),true)
       shared_libs += libsdsprpc
    else
       shared_libs += libadsprpc
    endif
endif

c_flags = -Werror -Wall -fexceptions
ifeq ($(LLVM_SENSORS_SEE),true)
c_flags += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif

ifeq ($(NO_SNS_FIRMWARE_SUPPORT_FOR_PROPERTY_READING), true)
c_flags += -DNO_FIRMWARE_SUPPORT_FOR_PROPERTY_READING
endif

c_flags += -Wno-unused-variable -Wno-unused-parameter
$(info $(c_flags))
ifeq ($(TARGET_SUPPORTS_WEARABLES),true)
c_flags +=-DSNS_WEARABLES_TARGET
c_flags +=-DTARGET_SUPPORTS_NO_HIFI
endif

ifeq ($(USE_SENSOR_MULTI_HAL),true)
    LOCAL_MODULE := sensors.ssc
else
    LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)
    LOCAL_MODULE_RELATIVE_PATH := hw
endif

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_CLANG := true
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES = $(c_sources)
LOCAL_SRC_FILES := $(filter-out framework/rpcmem_android.c, $(LOCAL_SRC_FILES))
LOCAL_CFLAGS   := $(c_flags)
ifeq ($(TARGET_SUPPORT_DIRECT_REPORT), true)
    SRC_C_LIST := $(LOCAL_PATH)/framework/rpcmem_android.c
    LOCAL_SRC_FILES += $(SRC_C_LIST:$(LOCAL_PATH)/%=%)
    LOCAL_CFLAGS += -DSNS_DIRECT_REPORT_SUPPORT
else
    LOCAL_SRC_FILES :=$(filter-out framework/direct_channel.cpp, $(LOCAL_SRC_FILES))
endif

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
    LOCAL_SRC_FILES :=$(filter-out sensors/ppg.cpp, $(LOCAL_SRC_FILES))
endif

ifeq ($(TARGET_USES_QMAA), true)
    ifeq ($(TARGET_USES_QMAA_OVERRIDE_SENSORS),true)
        LOCAL_SRC_FILES := $(filter-out framework/sensors_hal_stub.cpp, $(LOCAL_SRC_FILES))
    else
        LOCAL_SRC_FILES := $(filter-out framework/sensors_hal.cpp, $(LOCAL_SRC_FILES))
        LOCAL_CLANG_CFLAGS += -Wno-error=unused-private-field
    endif
else
    LOCAL_SRC_FILES := $(filter-out framework/sensors_hal_stub.cpp, $(LOCAL_SRC_FILES))
endif

LOCAL_C_INCLUDES := $(c_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(c_export_includes)
LOCAL_SHARED_LIBRARIES := $(shared_libs)

include $(BUILD_SHARED_LIBRARY)

ifeq ($(call is-board-platform-in-list,msmnile),true)
$(info "copying sensors_list file")
include $(CLEAR_VARS)
LOCAL_MODULE := sensors_list
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .txt
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/persist/sensors/
LOCAL_SRC_FILES := sensors_list.txt
include $(BUILD_PREBUILT)
endif
endif
