LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

c_sources =
c_sources += ssc_drva_test.cpp

c_includes =
c_includes += $(LOCAL_PATH)
# TODO: try to use automatic export_includes from libssc.so, to remove these
# dependencies
c_includes += $(TARGET_OUT_INTERMEDIATES)/../gen/SHARED_LIBRARIES/libssc_intermediates/proto
c_includes += vendor/qcom/proprietary/commonsys-intf/sensors-see/ssc
c_includes += vendor/qcom/proprietary/commonsys-intf/sensors-see/sensors-log
c_includes += $(TARGET_OUT_HEADERS)/qmi-framework/inc

c_flags = -Wno-unused-variable -Wno-unused-parameter -fexceptions
ifeq ($(LLVM_SENSORS_SEE),true)
c_flags += --compile-and-analyze --analyzer-perf --analyzer-Werror
endif


shared_libs =
shared_libs += libssc
shared_libs += liblog
shared_libs += libsensorslog
shared_libs += libprotobuf-cpp-full
shared_libs += libcutils

LOCAL_HEADER_LIBRARIES := libcutils_headers

LOCAL_MODULE := ssc_drva_test
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := $(c_sources)
LOCAL_C_INCLUDES := $(c_includes)
LOCAL_CFLAGS := $(c_flags)
LOCAL_SHARED_LIBRARIES := $(shared_libs)

include $(BUILD_EXECUTABLE)
