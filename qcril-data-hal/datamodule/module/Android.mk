LOCAL_PATH := $(call my-dir)

MY_LOCAL_PATH := $(LOCAL_PATH)

include $(call all-subdir-makefiles)

LOCAL_PATH := $(MY_LOCAL_PATH)

DATAMODULE_DIR := ${LOCAL_PATH}/..

DATA_HAL_DIR   := ${LOCAL_PATH}/../..

###################### Target ########################################
include $(CLEAR_VARS)

LOCAL_MODULE               := qcrilDataModule
LOCAL_SANITIZE:=integer_overflow
LOCAL_MODULE_OWNER         := qti
LOCAL_PROPRIETARY_MODULE   := true
LOCAL_MODULE_TAGS          := optional

LOCAL_CFLAGS               += -Wall -Werror -Wno-macro-redefined -DFEATURE_DATA_LOG_QXDM -DFEATURE_QDP_LINUX_ANDROID
LOCAL_CXXFLAGS             += -std=c++17

# Build with ASAN and unresolved reference flags enabled
ifeq ($(QCRIL_BUILD_WITH_ASAN),true)
LOCAL_LDFLAGS              += -Wl,--no-allow-shlib-undefined,--unresolved-symbols=report-all
LOCAL_SANITIZE             :=address
endif

LOCAL_SRC_FILES            := $(call all-cpp-files-under, src)
LOCAL_SRC_FILES            += $(call all-c-files-under, src)

LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../endpoints/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../authmanager/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../profilehandler/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../lcehandler/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../networkservicehandler/src)
LOCAL_SRC_FILES            += $(call all-c-files-under, ../../lqe/src)

# TODO Remove once qdphandler separated out
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../qdphandler/legacy)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../qdphandler/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../util/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../callmanager/src)

ifeq ($(TARGET_HAS_LOW_RAM),true)
LOCAL_CFLAGS               += -DRIL_FOR_LOW_RAM
LOCAL_CXXFLAGS             += -DRIL_FOR_LOW_RAM

LOCAL_SRC_FILES            := $(filter-out %DataCallListTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %DeactivateDataCallTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsActivateDeactivateTmgiTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsActivateTmgiTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsContentDescUpdateMessageTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsDeactivateTmgiTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsEnableDataReqTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsGetActiveTmgiTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsGetAvailTmgiTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsSendIntTmgiListTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %EmbmsCallHandler.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %LastDataCallFailCauseTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %PullLceDataTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %SetDataProfileTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %SetupDataCallTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %StartKeepaliveTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %StartLceTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %StopKeepaliveTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %StopLceTranslator.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %SegmentTracker.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %VoiceCallModemEndPoint.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %VoiceCallModemEndPointModule.cpp,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES            := $(filter-out %qcril_data_embms.c,$(LOCAL_SRC_FILES))
else
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../iwlanservice)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../KeepAliveManager/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../dataconnectionservice/src)
LOCAL_SRC_FILES            += $(call all-cpp-files-under, ../../networkavailabilityhandler/src)
LOCAL_SHARED_LIBRARIES     += vendor.qti.hardware.data.iwlan@1.0
LOCAL_SHARED_LIBRARIES     += vendor.qti.hardware.data.connection@1.0
LOCAL_SHARED_LIBRARIES     += vendor.qti.hardware.data.connection@1.1
endif

LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/lqe/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qcril/
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qcril/framework
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc

LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/data/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qmi-framework/inc
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qmi/inc
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

# Local
LOCAL_C_INCLUDES += $(DATAMODULE_DIR)/include
LOCAL_C_INCLUDES += $(DATAMODULE_DIR)/include/legacy
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/lcehandler/inc

LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/endpoints/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/authmanager/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/dataconnectionservice/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/profilehandler/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/networkservicehandler/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/networkavailabilityhandler/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/KeepAliveManager/inc

LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/qdphandler/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/qdphandler/legacy
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/util/inc
LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/callmanager/inc

LOCAL_C_INCLUDES += $(DATA_HAL_DIR)/iwlanservice

LOCAL_CFLAGS += -DQCRIL_DATA_MODULE_DEV

LOCAL_CFLAGS += -DFEATURE_QCRIL_USE_NETCTRL
LOCAL_CFLAGS += -DRIL_REQUEST_SET_INITIAL_ATTACH_APN
LOCAL_CFLAGS += -DFEATURE_DATA_EMBMS
LOCAL_CFLAGS += -DFEATURE_DATA_LQE

LOCAL_SHARED_LIBRARIES += libqdp
LOCAL_SHARED_LIBRARIES += libdsi_netctrl
LOCAL_SHARED_LIBRARIES += libril-qc-hal-qmi
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libdl
LOCAL_SHARED_LIBRARIES += libbase
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libhwbinder
LOCAL_SHARED_LIBRARIES += libhidlbase
LOCAL_SHARED_LIBRARIES += libhardware_legacy
LOCAL_SHARED_LIBRARIES += qtimutex
LOCAL_HEADER_LIBRARIES += qtimutex-headers

$(info Done building qcrilDataModule for target...)

include $(BUILD_STATIC_LIBRARY)

###################### Host ############################################
 include $(CLEAR_VARS)

 LOCAL_MODULE               := qcrilDataModule
LOCAL_SANITIZE:=integer_overflow
 LOCAL_MODULE_OWNER         := qti
 LOCAL_PROPRIETARY_MODULE   := true
 LOCAL_MODULE_TAGS          := optional

 LOCAL_CFLAGS               += -Wall -DHOST_EXECUTABLE_BUILD -g -fprofile-arcs -ftest-coverage --coverage -fexceptions
 LOCAL_CXXFLAGS             += -std=c++14
 LOCAL_SRC_FILES            := $(call all-cpp-files-under, src)
 LOCAL_SRC_FILES            += $(call all-c-files-under, src)

 LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qcril/
 LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qcril/framework/
 #LOCAL_C_EXPORT_INCLUDES += $(TARGET_OUT_HEADERS)/qcril/modules/qmi/
 LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/common/inc
 LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/data/inc
 LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/diag/include
 LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/qmi-framework/inc
 LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr
 LOCAL_C_INCLUDES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

 # Local
 LOCAL_C_INCLUDES += $(DATAMODULE_DIR)/include
 LOCAL_C_INCLUDES += $(DATAMODULE_DIR)/include/legacy

 # This is for development only
 LOCAL_CFLAGS += -DQCRIL_DATA_MODULE_DEV

 LOCAL_CFLAGS += -DFEATURE_QCRIL_USE_NETCTRL
 LOCAL_CFLAGS += -DRIL_REQUEST_SET_INITIAL_ATTACH_APN
 LOCAL_CFLAGS += -DFEATURE_DATA_EMBMS
 LOCAL_CFLAGS += -DFEATURE_DATA_LQE
 LOCAL_CFLAGS += -DQCRIL_DATA_OFFTARGET
 LOCAL_CFLAGS += -DFEATURE_DATA_LOG_STDERR
 LOCAL_CFLAGS += -DFEATURE_UNIT_TEST

 LOCAL_SHARED_LIBRARIES += libqcrilFramework
 LOCAL_SHARED_LIBRARIES += libril-qc-hal-qmi

 #$(info Done building qcrilDataModule for host...)

 #include $(BUILD_HOST_STATIC_LIBRARY)

 LOCAL_HEADER_LIBRARIES := qcrilDataModule_headers
 LOCAL_EXPORT_HEADER_LIBRARY_HEADERS := qcrilDataModule_headers

 include $(CLEAR_VARS)

 LOCAL_MODULE := init-qcril-data.rc

 LOCAL_MODULE_CLASS := ETC
 LOCAL_MODULE_PATH := $(OUT_DIR)/target/product/$(TARGET_DEVICE)/vendor/etc/init

 LOCAL_SRC_FILES := $(LOCAL_MODULE)

 LOCAL_MODULE_OWNER := qti
 LOCAL_PROPRIETARY_MODULE := true

 include $(BUILD_PREBUILT)
