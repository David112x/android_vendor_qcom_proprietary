ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/csl
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                        \
    camxcdmdefs.cpp                       \
    camxcsl.cpp                           \
    camxcsljumptable.cpp                  \
    common/camxcslcommonutils.cpp         \
    hw/camxcslhw.cpp                      \
    hw/camxcslhwicptypes.cpp              \
    hw/camxcslhwifetypes.cpp              \
    hw/camxcslhwinternal.cpp              \
    hw/camxcslhwinternalactuator.cpp      \
    hw/camxcslhwinternalcpas.cpp          \
    hw/camxcslhwinternalcsiphy.cpp        \
    hw/camxcslhwinternaleeprom.cpp        \
    hw/camxcslhwinternalfd.cpp            \
    hw/camxcslhwinternalois.cpp           \
    hw/camxcslhwinternalicp.cpp           \
    hw/camxcslhwinternalife.cpp           \
    hw/camxcslhwinternalcustomhw.cpp      \
    hw/camxcslhwinternaljpeg.cpp          \
    hw/camxcslhwinternallrme.cpp          \
    hw/camxcslhwinternalsensor.cpp        \
    hw/camxcslhwinternalvfe.cpp           \
    hw/camxcslhwinternalflash.cpp         \
    hw/camxcslhwlrmetypes.cpp             \
    hw/camxcslhwtypes.cpp                 \
    hw/camxcslhwvfetypes.cpp              \
    hw/camxsyncmanager.cpp                \
    ifh/camxcslifh.cpp                    \

LOCAL_INC_FILES :=              \
    camxcsl.h                   \
    camxcslfddefs.h             \
    camxcslicpdefs.h            \
    camxcsljpegdefs.h           \
    camxcsljumptable.h          \
    camxcslresourcedefs.h       \
    camxcslsensordefs.h         \
    camxcslvfedefs.h            \
    camxpacketdefs.h            \
    common/camxcslcommonutils.h \
    hw/camxcslhwinternal.h      \
    hw/camxsyncmanager.h        \

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES :=                                                \
    $(CAMX_C_INCLUDES)                                             \
    $(CAMX_PATH)/src/csl/common                                    \
    $(CAMX_PATH)/src/csl/ifh                                       \
    $(CAMX_PATH)/src/utils                                         \
    $(CAMX_OUT_HEADERS)/titan17x                                   \
    $(CAMX_OUT_HEADERS)/titan48x                                   \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include             \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/camera/include/uapi

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Library name
LOCAL_MODULE := libcamxcsl

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
LOCAL_COPY_HEADERS_TO := camx
LOCAL_COPY_HEADERS :=            \
    camxcsl.h                    \
    camxcslresourcedefs.h        \

include $(BUILD_COPY_HEADERS)
