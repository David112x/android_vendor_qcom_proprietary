LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libUSTANative
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

c_sources =
c_sources += sensor_cxt.cpp
c_sources += sensor_suid.cpp
c_sources += sensor.cpp
c_sources += sensor_descriptor_pool.cpp
c_sources += sensor_message_parser.cpp
c_sources += sensor_attributes.cpp
c_sources += logging_util.cpp
c_sources += com_qualcomm_qti_usta_core_SensorContextJNI.cpp

c_includes =
c_includes += $(LOCAL_PATH)
c_includes += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_intermediates/proto
c_includes += $(LOCAL_PATH)/../../ssc
c_includes += $(LOCAL_PATH)/../../sensors-diag-log/inc
c_includes += $(LOCAL_PATH)/../../sns_device_mode/inc
c_includes += $(LOCAL_PATH)/../../sns_fastRPC_utils/inc
c_includes += $(LOCAL_PATH)/../../../../adsprpc/inc/
c_includes += $(JNI_H_INCLUDE)

shared_libs =
shared_libs += libssc
shared_libs += liblog
shared_libs += libprotobuf-cpp-full
shared_libs += libsnsdiaglog
shared_libs += libutils libcutils
ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
shared_libs += libsns_device_mode_stub
shared_libs += libsns_fastRPC_util
endif

$(shell mkdir -p $(TARGET_OUT_ETC)/sensors/proto)

$(shell cp -f external/protobuf/src/google/protobuf/descriptor.proto $(TARGET_OUT_ETC)/sensors)
$(shell cp -f $(LOCAL_PATH)/../../ssc/proto/*.proto $(TARGET_OUT_ETC)/sensors)
$(shell mv -f $(TARGET_OUT_ETC)/sensors/*.proto $(TARGET_OUT_ETC)/sensors/proto)

c_flags = -Werror -Wno-unused-parameter -fexceptions
ifeq ($(LLVM_SENSORS_SEE),true)
c_flags += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif
ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
c_flags += -DENABLE_DEVICE_MODE
endif

## C/C++ compiler flags
LOCAL_CFLAGS   = $(c_flags)

LOCAL_CLANG := true
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES = $(c_sources)
LOCAL_CPPFLAGS = $(cpp_flags)
LOCAL_C_INCLUDES = $(c_includes)
LOCAL_SHARED_LIBRARIES = $(shared_libs)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libUSTANative_system
LOCAL_MODULE_CLASS := SHARED_LIBRARIES

c_sources =
c_sources += sensor_cxt.cpp
c_sources += sensor_suid.cpp
c_sources += sensor.cpp
c_sources += sensor_descriptor_pool.cpp
c_sources += sensor_message_parser.cpp
c_sources += sensor_attributes.cpp
c_sources += logging_util.cpp
c_sources += com_qualcomm_qti_usta_core_SensorContextJNI.cpp

c_includes =
c_includes += $(LOCAL_PATH)
c_includes += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_system_intermediates/proto
c_includes += $(LOCAL_PATH)/../../ssc
c_includes += $(LOCAL_PATH)/../../sensors-diag-log/inc
c_includes += $(LOCAL_PATH)/../../sns_device_mode/inc
c_includes += $(LOCAL_PATH)/../../sns_fastRPC_utils/inc/
c_includes += $(LOCAL_PATH)/../../../../adsprpc/inc/
c_includes += $(JNI_H_INCLUDE)

shared_libs =
shared_libs += libssc_system
shared_libs += liblog
shared_libs += libprotobuf-cpp-full
shared_libs += libsnsdiaglog_system
ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
shared_libs += libsns_device_mode_stub_system
shared_libs += libsns_fastRPC_util_system
endif

LOCAL_INSTALLED_MODULE_STEM := libUSTANative.so


c_flags = -Werror -Wno-unused-parameter -fexceptions
ifeq ($(LLVM_SENSORS_SEE),true)
c_flags += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif
ifneq ($(TARGET_SUPPORTS_WEARABLES),true)
c_flags += -DENABLE_DEVICE_MODE
endif

## C/C++ compiler flags
LOCAL_CFLAGS   = $(c_flags)

LOCAL_CLANG := true
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_SRC_FILES = $(c_sources)
LOCAL_CPPFLAGS = $(cpp_flags)
LOCAL_C_INCLUDES = $(c_includes)
LOCAL_SHARED_LIBRARIES = $(shared_libs)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

# List of power scripts
sns_power_scripts_files := $(wildcard $(LOCAL_PATH)/power_scripts/*.txt)
$(shell mkdir -p $(TARGET_OUT_DATA)/sensors/scripts)
$(info "Adding power scripts files  = $(sns_power_scripts_files)")
define _add_power_files
 include $$(CLEAR_VARS)
 LOCAL_MODULE := $(notdir $(1))
 LOCAL_MODULE_STEM := $(notdir $(1))
 sns_power_scripts_modules += $$(LOCAL_MODULE)
 LOCAL_SRC_FILES := $(1:$(LOCAL_PATH)/%=%)
 LOCAL_MODULE_TAGS := optional
 LOCAL_MODULE_CLASS := ETC
 LOCAL_MODULE_PATH := $$(TARGET_OUT_DATA)/sensors/scripts
include $$(BUILD_PREBUILT)
endef

sns_power_scripts_modules :=
sns_config :=
$(foreach _conf-file, $(sns_power_scripts_files), $(eval $(call _add_power_files,$(_conf-file))))
