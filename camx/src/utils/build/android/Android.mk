ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/utils
endif
include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                  \
    camxatomic.cpp                  \
    camxdebug.cpp                   \
    camxdebugprint.cpp              \
    camxhashmap.cpp                 \
    camximagedump.cpp               \
    camxmemspy.cpp                  \
    camxnodeutils.cpp               \
    camxstabilization.cpp           \
    camxthreadcore.cpp              \
    camxthreadjoblist.cpp           \
    camxthreadjobregistry.cpp       \
    camxthreadmanager.cpp           \
    camxthreadqueue.cpp             \
    camxtrace.cpp                   \
    camxtranslator.cpp              \

LOCAL_INC_FILES :=                  \
    camxatomic.h                    \
    camxdebug.h                     \
    camxdebugprint.h                \
    camxdefs.h                      \
    camxhashmap.h                   \
    camximagedump.h                 \
    camxincs.h                      \
    camxlist.h                      \
    camxmemspy.h                    \
    camxnodeutils.h                 \
    camxstabilization.h             \
    camxthreadcommon.h              \
    camxthreadcore.h                \
    camxthreadjoblist.h             \
    camxthreadjobregistry.h         \
    camxthreadmanager.h             \
    camxthreadqueue.h               \
    camxtrace.h                     \
    camxtranslator.h                \
    camxtypes.h                     \
    camxutils.h                     \

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)

# Paths to included headers
LOCAL_C_INCLUDES       := $(CAMX_C_INCLUDES)              \
                          $(CAMX_CHICDK_API_PATH)/utils   \
                          $(TARGET_OUT_HEADERS)/camx

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_SHARED_LIBRARIES +=        \
     libcamximageformatutils

# Binary name
LOCAL_MODULE := libcamxutils

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)

# COPY below headers to out\target\product\<target>\obj\include\camx
LOCAL_COPY_HEADERS_TO := camx
LOCAL_COPY_HEADERS :=   camxatomic.h                    \
                        camxdebug.h                     \
                        camxdebugprint.h                \
                        camxdefs.h                      \
                        camxhashmap.h                   \
                        camxincs.h                      \
                        camxlist.h                      \
                        camxmemspy.h                    \
                        camxthreadcommon.h              \
                        camxthreadcore.h                \
                        camxthreadjoblist.h             \
                        camxthreadjobregistry.h         \
                        camxthreadmanager.h             \
                        camxthreadqueue.h               \
                        camxtrace.h                     \
                        camxtypes.h                     \
                        camxutils.h                     \

include $(BUILD_COPY_HEADERS)
