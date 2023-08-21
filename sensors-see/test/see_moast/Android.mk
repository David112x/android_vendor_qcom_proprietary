LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

c_sources =
c_sources += main.cpp
c_sources += Parser.cpp
c_sources += Utility.cpp
c_sources += Moast.cpp
c_sources += Extras.cpp

c_includes =
c_includes += $(LOCAL_PATH)
c_includes += $(LOCAL_PATH)/../see_salt

c_flags = -Wno-unused-variable -Werror -Wno-unused-parameter -fexceptions -g -std=c++11 -pthread

shared_libs =
shared_libs += libSEESalt

LOCAL_MODULE := see_moast
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := $(c_sources)
LOCAL_C_INCLUDES := $(c_includes)
LOCAL_CFLAGS := $(c_flags)
LOCAL_SHARED_LIBRARIES := $(shared_libs)

include $(BUILD_EXECUTABLE)
