LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE:=sns_hal_batch
LOCAL_MODULE_OWNER:=qti

ifeq ($(USE_SENSOR_HAL_VER),2.0)
LOCAL_C_INCLUDES = $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../hal-2.0-hidl-impl/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../sensors-hal-2.0/framework/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../commonsys-intf/sensors-see/ssc/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)
LOCAL_SHARED_LIBRARIES += \
  libhardware \
  libbase \
  libfmq \
  libcutils \
  libutils \
  liblog \
  libhidlbase \
  libssc \
  libhidltransport \
  libsensorslog \
  libbinder \
  libhwbinder \
  android.hardware.sensors@2.0 \
  android.hardware.sensors@2.0-impl \
  libc \
  sensors.ssc

LOCAL_SRC_FILES += \
   sns_hal_batch_fmq.cpp \
   convert.cpp

else
LOCAL_SHARED_LIBRARIES += \
    libdl \
    libhardware

LOCAL_SRC_FILES += \
    sns_espplus_testapp.c
endif

ifeq ($(USE_SENSOR_HAL_VER),2.0)
LOCAL_CFLAGS += -DUSE_SENSOR_HAL_VER_2_0
endif

LOCAL_CFLAGS += -DBOARD_PLATFORM=\"$(TARGET_BOARD_PLATFORM)\"

ifeq ($(USE_SENSOR_MULTI_HAL),true)
LOCAL_CFLAGS += -DUSES_MULTI_HAL=\"$(USE_SENSOR_MULTI_HAL)\"
endif

ifeq ($(PLATFORM_VERSION), P)
LOCAL_CFLAGS += -DUSES_FULL_TREBLE
endif

ifeq ($(TARGET_SUPPORTS_WEARABLES),true)
LOCAL_CFLAGS += -DSNS_WEARABLES_TARGET
endif

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
include $(CLEAR_VARS)
LOCAL_MODULE:=sns_hidl_hal_batch
LOCAL_MODULE_OWNER:=qti

ifeq ($(USE_SENSOR_HAL_VER),2.0)
LOCAL_C_INCLUDES = $(LOCAL_PATH)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../hal-2.0-hidl-impl/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../sensors-hal-2.0/framework/
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../commonsys-intf/sensors-see/ssc/

LOCAL_SHARED_LIBRARIES += \
    liblog \
    libutils \
    libcutils \
    libhidlbase \
    libhidltransport \
    libhardware \
    libhwbinder \
    libbase \
    libfmq \
    libssc \
    libbinder \
    libhwbinder \
    android.hardware.sensors@2.0 \
    sensors.ssc \
    libsensorslog

LOCAL_SRC_FILES += \
 sns_hidl_hal_batch_fmq.cpp \
 convert.cpp

LOCAL_CFLAGS += -DUSE_SENSOR_HAL_VER_2_0
LOCAL_CFLAGS += -DBOARD_PLATFORM=\"$(TARGET_BOARD_PLATFORM)\"
LOCAL_CFLAGS += -D_GNU_SOURCE -Wall -Werror -Wno-unused-parameter

ifeq ($(LLVM_SENSORS_SEE),true)
LOCAL_CFLAGS += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif

else
LOCAL_SHARED_LIBRARIES += \
    liblog \
    libutils \
    libcutils \
    libhidlbase \
    libhidltransport \
    libhardware \
    libhwbinder \
    android.hardware.sensors@1.0 \
    libdl \
    libbase
LOCAL_SRC_FILES += \
    sns_hidl_hal_batch.cpp \
    convert.cpp
LOCAL_CFLAGS += -DUSE_SENSOR_HAL_VER_2_0
LOCAL_CFLAGS += -DBOARD_PLATFORM=\"$(TARGET_BOARD_PLATFORM)\"
LOCAL_CFLAGS += -D_GNU_SOURCE -Wall -Werror -Wno-unused-parameter

ifeq ($(LLVM_SENSORS_SEE),true)
LOCAL_CFLAGS += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif
endif

LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := \
    libsensor
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
endif
