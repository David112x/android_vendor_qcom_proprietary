LOCAL_PATH := $(call my-dir)

# =================================
# copy header
# =================================
include $(CLEAR_VARS)

#LOCAL_COPY_HEADERS_TO := qti-utils/inc
#LOCAL_COPY_HEADERS    := qti-utils.h
LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := qti-utils

#include $(BUILD_COPY_HEADERS)

# =================================
# libqti-utils.a
# =================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    memsutils.c

LOCAL_MODULE := libqti-utils
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MULTILIB := both

include $(BUILD_STATIC_LIBRARY)

# =================================
# libqti-utils.so
# =================================

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    memsutils.c

LOCAL_MODULE := libqti-utils
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MULTILIB := both

include $(BUILD_SHARED_LIBRARY)
