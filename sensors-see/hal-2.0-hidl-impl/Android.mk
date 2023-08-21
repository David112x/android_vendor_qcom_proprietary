ifeq ($(USE_SENSOR_HAL_VER),2.0)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.sensors@2.0-impl
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_CFLAGS   := -DUSE_SENSOR_HAL_VER_2_0
LOCAL_CFLAGS += -Werror -Wno-unused-variable -Wno-unused-parameter
LOCAL_C_INCLUDES = $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../hal/framework/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)
ifeq ($(call is-board-platform-in-list,sdm845 msmnile sdm710 qcs605 $(MSMSTEPPE) kona lito atoll),true)
LOCAL_CFLAGS += -DSNS_DIRECT_REPORT_SUPPORT
LOCAL_C_INCLUDES += vendor/qcom/proprietary/sensors-see/sns_low_lat_vendor/inc
LOCAL_C_INCLUDES += vendor/qcom/proprietary/commonsys-intf/sensors-see/sns_fastRPC_utils/inc
endif

ifeq ($(TARGET_USES_QMAA), true)
    ifeq ($(TARGET_USES_QMAA_OVERRIDE_SENSORS),true)
        LOCAL_SRC_FILES := sensors_hw_module.cpp
    else
        LOCAL_SRC_FILES := sensors_hw_module_stub.cpp
        LOCAL_CLANG_CFLAGS += -Wno-error=unused-private-field
    endif
else
    LOCAL_SRC_FILES := sensors_hw_module.cpp
endif

LOCAL_SHARED_LIBRARIES := \
    libhidlbase \
    libhidltransport \
    libutils \
    libdl \
    liblog \
    libcutils \
    libhardware \
    libbase \
    android.hardware.sensors@2.0 \
    libc \
    libfmq \
    libssc \
    libsensorslog \
    sensors.ssc

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_OWNER := qti
LOCAL_CFLAGS := -Werror -Wno-unused-variable -Wno-unused-parameter
LOCAL_C_INCLUDES = $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../hal/framework/
LOCAL_MODULE := android.hardware.sensors@2.0-service
LOCAL_INIT_RC := android.hardware.sensors@2.0-service.rc
LOCAL_SRC_FILES := \
    service.cpp

LOCAL_SHARED_LIBRARIES := \
        liblog \
        libcutils \
        libdl \
        libbase \
        libutils \
        libfmq \
        libhardware \
        libsensorslog \
        libssc \
        sensors.ssc \

LOCAL_SHARED_LIBRARIES += \
        libhidlbase \
        libhidltransport \
        android.hardware.sensors@2.0 \
        android.hardware.sensors@2.0-impl

include $(BUILD_EXECUTABLE)
endif
