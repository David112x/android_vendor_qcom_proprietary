ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/swl/stats
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                      \
    camxaecengine.cpp                   \
    camxautofocusnode.cpp               \
    camxafstatemachine.cpp              \
    camxawbnode.cpp                     \
    camxcaecstatsprocessor.cpp          \
    camxcafalgorithmhandler.cpp         \
    camxcafdalgorithmhandler.cpp        \
    camxhistalgorithmhandler.cpp        \
    camxcafioutil.cpp                   \
    camxcafdstatsprocessor.cpp          \
    camxcafdiohandler.cpp               \
    camxcafstatsprocessor.cpp           \
    camxcasdstatsprocessor.cpp          \
    camxcawbioutil.cpp                  \
    camxcawbstatsprocessor.cpp          \
    camxhistogramprocessnode.cpp        \
    camxmultistatsoperator.cpp          \
    camxstatscommon.cpp                 \
    camxstatsdebug.cpp                  \
    camxstatsparsenode.cpp              \
    camxstatsprocessingnode.cpp         \
    camxstatsprocessormanager.cpp       \
    camxtrackernode.cpp

LOCAL_INC_FILES :=                                          \
    camxaecengine.h                                         \
    camxautofocusnode.h                                     \
    camxafstatemachine.h                                    \
    camxawbnode.h                                           \
    camxcaecstatsprocessor.h                                \
    camxcafalgorithmhandler.h                               \
    camxhistalgorithmhandler.h                               \
    camxcafioutil.h                                         \
    camxcafdstatsprocessor.h                                \
    camxcafdalgorithmhandler.h                              \
    camxcafdiohandler.h                                     \
    camxcafstatsprocessor.h                                 \
    camxcasdstatsprocessor.h                                \
    camxcawbstatsprocessor.h                                \
    camxcawbioutil.h                                        \
    camxhistogramprocessnode.h                              \
    camxmultistatsoperator.h                                \
    camxstatscommon.h                                       \
    camxstatsdebuginternal.h                                \
    camxstatsparsenode.h                                    \
    camxstatsprocessingnode.h                               \
    camxstatsprocessormanager.h                             \
    camxtrackernode.h


# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS) \

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)              \
    system/media/camera/include                     \
    $(CAMX_CHICDK_API_PATH)/fd                      \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix   \
    $(CAMX_CHICDK_API_PATH)/generated/g_parser      \
    $(CAMX_CHICDK_API_PATH)/pdlib                   \
    $(CAMX_CHICDK_API_PATH)/stats                   \
	$(CAMX_CHICDK_API_PATH)/utils                   \
    $(CAMX_PATH)/ext                                \
    $(CAMX_PATH)/src/core/ncs                       \
    $(CAMX_PATH)/src/hwl/titan17x                   \
    $(CAMX_PATH)/src/hwl/iqinterpolation            \
    $(CAMX_PATH)/src/hwl/iqsetting

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

LOCAL_CFLAGS += -Wfloat-conversion         \
    -Wunused-variable                \
    -Wformat                         \
    -Wuninitialized                  \
    -Wunused-value                   \
    -Wunused-comparison              \
    -Wshadow

LOCAL_CPPFLAGS += -Wfloat-conversion         \
    -Wunused-variable                \
    -Wformat                         \
    -Wuninitialized                  \
    -Wunused-value                   \
    -Wunused-comparison              \
    -Wshadow

# Binary name
LOCAL_MODULE := libcamxstats

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
