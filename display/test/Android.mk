# ---------------------------------------------------------------------------------
#                       Make the postproc tester (ppd)
# ---------------------------------------------------------------------------------

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE        := ppd
LOCAL_MODULE_TAGS   := optional
LOCAL_CFLAGS        := -Werror -Wno-undefined-bool-conversion -Wno-format
LOCAL_CLANG         := true

LOCAL_SRC_FILES     += ppd.cpp

LOCAL_SHARED_LIBRARIES := libcutils liblog
LOCAL_SHARED_LIBRARIES += libhidlbase vendor.display.postproc@1.0

LOCAL_MODULE_OWNER := qti
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_EXECUTABLE)
