ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/ispiqmodule
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                  \
    camxbpsabf40.cpp                \
    camxbpsawbbgstats14.cpp         \
    camxbpsbpcpdpc20.cpp            \
    camxbpsbpcpdpc30.cpp            \
    camxbpscc13.cpp                 \
    camxbpscst12.cpp                \
    camxbpsdemosaic36.cpp           \
    camxbpsdemux13.cpp              \
    camxbpsgamma16.cpp              \
    camxbpsgic30.cpp                \
    camxbpsgtm10.cpp                \
    camxbpshdr22.cpp                \
    camxbpshdr30.cpp                \
    camxbpshdrbhiststats13.cpp      \
    camxbpshnr10.cpp                \
    camxbpslinearization34.cpp      \
    camxbpslsc34.cpp                \
    camxbpslsc40.cpp                \
    camxbpspedestal13.cpp           \
    camxbpswb13.cpp                 \
    camxifeabf34.cpp                \
    camxifeabf40.cpp                \
    camxifeawbbgstats14.cpp         \
    camxifebfstats23.cpp            \
    camxifebfstats24.cpp            \
    camxifebfstats25.cpp            \
    camxifebhiststats14.cpp         \
    camxifebls12.cpp                \
    camxifebpcbcc50.cpp             \
    camxifecamif.cpp                \
    camxifecamiflite.cpp            \
    camxifecc12.cpp                 \
    camxifecc13.cpp                 \
    camxifecrop10.cpp               \
    camxifecsid.cpp                 \
    camxifecrop11.cpp               \
    camxifecst12.cpp                \
    camxifecsstats14.cpp            \
    camxifedemosaic36.cpp           \
    camxifedemosaic37.cpp           \
    camxifedemux13.cpp              \
    camxifeds410.cpp                \
    camxifeds411.cpp                \
    camxifedsx10.cpp                \
    camxifedualpd10.cpp             \
    camxifegamma16.cpp              \
    camxifegtm10.cpp                \
    camxifehdr20.cpp                \
    camxifehdr22.cpp                \
    camxifehdr23.cpp                \
    camxifehdr30.cpp                \
    camxifehdrbestats15.cpp         \
    camxifehdrbhiststats13.cpp      \
    camxifehvx.cpp                  \
    camxifeihiststats12.cpp         \
    camxifelinearization33.cpp      \
    camxifelinearization34.cpp      \
    camxifelcr10.cpp                \
    camxifelsc34.cpp                \
    camxifelsc40.cpp                \
    camxifemnds16.cpp               \
    camxifemnds21.cpp               \
    camxifepdaf20.cpp               \
    camxifepdpc11.cpp               \
    camxifepdpc30.cpp               \
    camxifepedestal13.cpp           \
    camxifeprecrop10.cpp            \
    camxifer2pd10.cpp               \
    camxiferoundclamp11.cpp         \
    camxifersstats14.cpp            \
    camxifetintlessbgstats15.cpp    \
    camxifewb12.cpp                 \
    camxifewb13.cpp                 \
    camxipe2dlut10.cpp              \
    camxipeanr10.cpp                \
    camxipeasf30.cpp                \
    camxipecac22.cpp                \
    camxipecac23.cpp                \
    camxipechromaenhancement12.cpp  \
    camxipechromasuppression20.cpp  \
    camxipecolortransform12.cpp     \
    camxipecolorcorrection13.cpp    \
    camxipegamma15.cpp              \
    camxipegrainadder10.cpp         \
    camxipehnr10.cpp                \
    camxipeica10.cpp                \
    camxipeica20.cpp                \
    camxipeica30.cpp                \
    camxipelenr10.cpp               \
    camxipeltm13.cpp                \
    camxipeltm14.cpp                \
    camxipesce11.cpp                \
    camxipetf10.cpp                 \
    camxipetf20.cpp                 \
    camxipeupscaler12.cpp           \
    camxipeupscaler20.cpp           \
    camxiqinterface.cpp             \
    camxswtmc11.cpp                 \
    camxswtmc12.cpp

LOCAL_INC_FILES :=                  \
    camxbpsabf40.h                  \
    camxbpsawbbgstats14.h           \
    camxbpsbpcpdpc20.h              \
    camxbpsbpcpdpc30.h              \
    camxbpscc13.h                   \
    camxbpscst12.h                  \
    camxbpsdemosaic36.h             \
    camxbpsdemux13.h                \
    camxbpsgamma16.h                \
    camxbpsgic30.h                  \
    camxbpsgtm10.h                  \
    camxbpshdr22.h                  \
    camxbpshdr30.h                  \
    camxbpshnr10.h                  \
    camxbpshdrbhiststats13.h        \
    camxbpslinearization34.h        \
    camxbpslsc34.h                  \
    camxbpslsc40.h                  \
    camxbpspedestal13.h             \
    camxbpswb13.h                   \
    camxifeabf34.h                  \
    camxifeabf40.h                  \
    camxifeawbbgstats14.h           \
    camxifebfstats23.h              \
    camxifebfstats24.h              \
    camxifebfstats25.h              \
    camxifebhiststats14.h           \
    camxifebls12.h                  \
    camxifebpcbcc50.h               \
    camxifecamif.h                  \
    camxifecamiflite.h              \
    camxifecc12.h                   \
    camxifecc13.h                   \
    camxifecrop10.h                 \
    camxifecrop11.h                 \
    camxifecsstats14.h              \
    camxifecsid.h                   \
    camxifecst12.h                  \
    camxifedemosaic36.h             \
    camxifedemosaic37.h             \
    camxifedemux13.h                \
    camxifeds410.h                  \
    camxifeds411.h                  \
    camxifedsx10.h                  \
    camxifedualpd10.h               \
    camxifegamma16.h                \
    camxifegtm10.h                  \
    camxifehdr20.h                  \
    camxifehdr23.h                  \
    camxifehdr30.h                  \
    camxifehdrbestats15.h           \
    camxifehdrbhiststats13.h        \
    camxifehvx.h                    \
    camxifeihiststats12.h           \
    camxifelinearization33.h        \
    camxifelinearization34.h        \
    camxifelcr10.h                  \
    camxifelsc34.h                  \
    camxifelsc40.h                  \
    camxifemnds16.h                 \
    camxifemnds21.h                 \
    camxifepdaf20.h                 \
    camxifepdpc11.h                 \
    camxifepdpc30.h                 \
    camxifepedestal13.h             \
    camxifeprecrop10.h              \
    camxifer2pd10.h                 \
    camxiferoundclamp11.h           \
    camxifersstats14.h              \
    camxifetintlessbgstats15.h      \
    camxifewb12.h                   \
    camxifewb13.h                   \
    camxipe2dlut10.h                \
    camxipeanr10.h                  \
    camxipeasf30.h                  \
    camxipecac22.h                  \
    camxipecac23.h                  \
    camxipechromaenhancement12.h    \
    camxipechromasuppression20.h    \
    camxipecolortransform12.h       \
    camxipecolorcorrection13.h      \
    camxipegamma15.h                \
    camxipegrainadder10.h           \
    camxipehnr10.h                  \
    camxipeica10.h                  \
    camxipeica20.h                  \
    camxipeica30.h                  \
    camxipeicatestdata.h            \
    camxipelenr10.h                 \
    camxipeltm13.h                  \
    camxipeltm14.h                  \
    camxipesce11.h                  \
    camxipetf10.h                   \
    camxipetf20.h                   \
    camxipeupscaler12.h             \
    camxipeupscaler20.h             \
    camxiqinterface.h               \
    camxispiqmodule.h               \
    camxswtmc11.h                   \
    camxswtmc12.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers

LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                              \
    $(CAMX_PATH)/src/core                                           \
    $(CAMX_CHICDK_API_PATH)/generated/g_parser                      \
    $(CAMX_PATH)/src/hwl/dspinterfaces                              \
    $(CAMX_PATH)/src/hwl/isphwsetting                               \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan17x                      \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan480                      \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan17x/bps                  \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan480/bps                  \
    $(CAMX_PATH)/src/hwl/ispiqmodule                                \
    $(CAMX_PATH)/src/hwl/ife                                        \
    $(CAMX_PATH)/src/hwl/ipe                                        \
    $(CAMX_PATH)/src/hwl/iqsetting                                  \
    $(CAMX_PATH)/src/hwl/titan17x                                   \
    $(CAMX_OUT_HEADERS)                                             \
    $(CAMX_OUT_HEADERS)/titan17x                                    \
    $(CAMX_OUT_HEADERS)/titan48x                                    \

ifeq ($(IQSETTING),OEM1)
LOCAL_C_INCLUDES +=                                                 \
    $(CAMX_OEM1IQ_PATH)/iqsetting
else
LOCAL_C_INCLUDES +=                                                 \
    $(CAMX_PATH)/src/hwl/iqinterpolation
endif

LOCAL_C_INCLUDES +=                                                 \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxhwliqmodule

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
