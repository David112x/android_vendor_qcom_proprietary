ifeq ($(CAMX_CHICDK_CORE_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../../..)
CAMX_CHICDK_CORE_PATH := $(abspath $(LOCAL_PATH)/..)
else
LOCAL_PATH := $(CAMX_CHICDK_CORE_PATH)/lib
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_CHICDK_PATH)/core/build/android/common.mk

LOCAL_INC_FILES :=

LOCAL_SRC_FILES :=

LOCAL_C_LIBS := $(CAMX_C_LIBS)

LOCAL_C_INCLUDES :=                 \
    $(CAMX_C_INCLUDES)

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

COMPOSER_INPUT         := $(CAMX_CHICDK_TOPOLOGY_PATH)/$(TARGET_BOARD_PLATFORM)/$(TARGET_BOARD_PLATFORM)_usecase.xml
COMPOSER_OUTPUT        := $(CAMX_CHICDK_TOPOLOGY_PATH)/$(TARGET_BOARD_PLATFORM)/g_$(TARGET_BOARD_PLATFORM)_usecase.xml
COPYRIGHT_HEADER       := $(CAMX_CHICDK_PATH)/tools/usecaseconverter/xmlheader.txt

# Set dependencies
$(COMPOSER_OUTPUT) : .KATI_IMPLICIT_OUTPUTS := $(COMPOSER_INPUT) $(COPYRIGHT_HEADER)

# UsecaseConverter definitions
COMMON_USECASE   := $(CAMX_CHICDK_TOPOLOGY_PATH)/common/common_usecase.xml
G_PIPELINES_FILE := $(LOCAL_PATH)/common/g_pipelines.h
CONVERTER_CMD    := perl $(CAMX_CHICDK_PATH)/tools/usecaseconverter/usecaseconverter.pl $(COMMON_USECASE) $(COMPOSER_OUTPUT) $(G_PIPELINES_FILE)

# Run UsecaseConverter to generate g_pipelines.h
CONVERTER_STATUS := $(shell $(CONVERTER_CMD)|| echo [UsecaseConverterFailure])
$(info $(CONVERTER_STATUS))
ifeq ($(lastword $(CONVERTER_STATUS)),[UsecaseConverterFailure])
    # Stop the build when error status code detected
    $(error $(G_PIPELINES_FILE) was not generated)
endif

# Set dependencies
$(G_PIPELINES_FILE) : .KATI_IMPLICIT_OUTPUTS += $(COMMON_USECASE) $(COMPOSER_OUTPUT)

LOCAL_STATIC_LIBRARIES :=

LOCAL_WHOLE_STATIC_LIBRARIES :=     \
    libchifeature2                  \
    libchiframework                 \
    libchiutils                     \
    libchiusecase

LOCAL_SHARED_LIBRARIES +=                                   \
    libcamera_metadata                                      \
    libchilog                                               \
    libcutils                                               \
    libhardware                                             \
    libhidlbase                                             \
    liblog                                                  \
    libqdMetaData                                           \
    libsync                                                 \
    libutils                                                \
    vendor.qti.hardware.camera.postproc@1.0-service-impl    \
    vendor.qti.hardware.vpp@1.1                             \
    vendor.qti.hardware.vpp@1.2

# Libraries to link
# Build in vendor partition
LOCAL_PROPRIETARY_MODULE := true

# Deployment path under lib/lib64
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE := com.qti.chi.override

include $(BUILD_SHARED_LIBRARY)
