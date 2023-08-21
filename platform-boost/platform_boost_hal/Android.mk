ifeq ($(TARGET_ENABLE_PLATFORM_BOOST),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := vendor.qti.platform_boost@1.0-service
LOCAL_INIT_RC := vendor.qti.platform_boost@1.0-service.rc
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_OWNER := qti

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/inc \
        $(TARGET_OUT_HEADERS)/common/inc \
        vendor/qcom/proprietary/android-perf/mp-ctl

LOCAL_SRC_FILES := $(call all-subdir-cpp-files)

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \
    libhidlbase \
    libhwbinder \
    libhidltransport \
    libtinyxml2 \
    vendor.qti.platform_boost@1.0

include $(BUILD_EXECUTABLE)

$(shell mkdir -p $(TARGET_OUT_VENDOR_ETC)/platform_boost)

# Create symbolic links for boost_mode.xml
$(shell cp -f $(LOCAL_PATH)/xml/boostMode/boost_mode.xml  $(TARGET_OUT_VENDOR_ETC)/platform_boost/)

# Create symbolic links for boost_cap.xml
# If Platform file not exist, then use common one.
$(shell cp -f $(LOCAL_PATH)/xml/boostCap/common/boost_cap.xml  $(TARGET_OUT_VENDOR_ETC)/platform_boost/)
$(shell cp -f $(LOCAL_PATH)/xml/boostCap/$(TARGET_BOARD_PLATFORM)/boost_cap.xml  $(TARGET_OUT_VENDOR_ETC)/platform_boost/)
endif