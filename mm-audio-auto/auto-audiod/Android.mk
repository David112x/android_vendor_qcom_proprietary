LOCAL_PATH:= $(call my-dir)

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_AUTO_AUDIOD)), true)

AUDIO_HAL_EXT := true
VEHICLE_HAL_EXT := true

include $(CLEAR_VARS)

ifneq ($(filter sdmshrike msmnile,$(TARGET_BOARD_PLATFORM)),)
	LOCAL_CFLAGS := -DPLATFORM_MSMNILE
endif
ifneq ($(filter $(MSMSTEPPE),$(TARGET_BOARD_PLATFORM)),)
	LOCAL_CFLAGS := -DPLATFORM_MSMSTEPPE
endif

LOCAL_MODULE:= audiod
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin

LOCAL_SRC_FILES:= \
	auto_audiod_main.cpp \
	AutoAudioDaemon.cpp \
	auto_audio_ext.cpp \

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	liblog \
	libbinder \
	libtinyalsa \
	libhidlbase \
	libhidltransport \
	libhwbinder \

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \

ifeq ($(AUDIO_HAL_EXT), true)
	LOCAL_CFLAGS += -DAHAL_EXT
	LOCAL_SHARED_LIBRARIES += android.hardware.audio@5.0 \
		android.hardware.audio.common@5.0
	LOCAL_C_INCLUDES += $(TOP)/android/hardware/audio/5.0
endif

ifeq ($(VEHICLE_HAL_EXT), true)
	LOCAL_CFLAGS += -DVHAL_EXT
	LOCAL_SRC_FILES += AutoPower.cpp
	LOCAL_SHARED_LIBRARIES += android.hardware.automotive.vehicle@2.0
	LOCAL_C_INCLUDES += hardware/interfaces/automotive/vehicle/2.0/default/common/include
endif

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

include $(BUILD_EXECUTABLE)

endif
