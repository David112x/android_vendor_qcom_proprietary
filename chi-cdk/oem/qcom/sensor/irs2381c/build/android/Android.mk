ifeq ($(CAMX_CHICDK_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_CHICDK_PATH := $(abspath $(LOCAL_PATH)/../../../..)
else
LOCAL_PATH := $(CAMX_CHICDK_PATH)/oem/qcom/sensor/irs2381c
endif

include $(CLEAR_VARS)
LOCAL_MODULE               := libdepthmapwrapper
LOCAL_MODULE_SUFFIX        := .so
LOCAL_MODULE_CLASS         := SHARED_LIBRARIES
LOCAL_SRC_FILES            := libdepthmapwrapper.so
LOCAL_MODULE_TAGS          := optional
LOCAL_MODULE_OWNER         := pmd
LOCAL_PROPRIETARY_MODULE   := true
LOCAL_MODULE_RELATIVE_PATH := $(CAMX_LIB_OUTPUT_PATH)
include $(PREBUILT_SHARED_LIBRARY)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE             := libdepthcomputation
LOCAL_MODULE_SUFFIX      := .so
LOCAL_MODULE_CLASS       := SHARED_LIBRARIES
LOCAL_SRC_FILES          := libdepthcomputation.so
LOCAL_MODULE_TAGS        := optional
LOCAL_MODULE_OWNER       := pmd
LOCAL_PROPRIETARY_MODULE := true
include $(PREBUILT_SHARED_LIBRARY)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE             := libeepromcutter
LOCAL_MODULE_SUFFIX      := .so
LOCAL_MODULE_CLASS       := SHARED_LIBRARIES
LOCAL_SRC_FILES          := libeepromcutter.so
LOCAL_MODULE_TAGS        := optional
LOCAL_MODULE_OWNER       := pmd
LOCAL_PROPRIETARY_MODULE := true
include $(PREBUILT_SHARED_LIBRARY)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE             := libspectre
LOCAL_MODULE_SUFFIX      := .so
LOCAL_MODULE_CLASS       := SHARED_LIBRARIES
LOCAL_SRC_FILES          := libspectre.so
LOCAL_MODULE_TAGS        := optional
LOCAL_MODULE_OWNER       := pmd
LOCAL_PROPRIETARY_MODULE := true
include $(PREBUILT_SHARED_LIBRARY)
include $(BUILD_PREBUILT)
