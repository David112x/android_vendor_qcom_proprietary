ifeq ($(CAMX_PATH),)
LOCAL_PATH := $(abspath $(call my-dir)/../..)
CAMX_PATH := $(abspath $(LOCAL_PATH)/../../..)
else
LOCAL_PATH := $(CAMX_PATH)/src/hwl/isphwsetting
endif

include $(CLEAR_VARS)

# Module supports function call tracing via ENABLE_FUNCTION_CALL_TRACE
# Required before including common.mk
SUPPORT_FUNCTION_CALL_TRACE := 1

# Get definitions common to the CAMX project here
include $(CAMX_PATH)/build/infrastructure/android/common.mk

LOCAL_SRC_FILES :=                                  \
    pipeline/camxisppipeline.cpp                    \
    pipeline/bps/camxbpspipelinetitan150.cpp        \
    pipeline/bps/camxbpspipelinetitan170.cpp        \
    pipeline/bps/camxbpspipelinetitan160.cpp        \
    pipeline/bps/camxbpspipelinetitan175.cpp        \
    pipeline/bps/camxbpspipelinetitan480.cpp        \
    pipeline/ife/camxtitan150ife.cpp                \
    pipeline/ife/camxtitan170ife.cpp                \
    pipeline/ife/camxtitan175ife.cpp                \
    pipeline/ife/camxtitan480ife.cpp                \
    pipeline/ipe/camxipepipelinetitan150.cpp        \
    pipeline/ipe/camxipepipelinetitan170.cpp        \
    pipeline/ipe/camxipepipelinetitan160.cpp        \
    pipeline/ipe/camxipepipelinetitan175.cpp        \
    pipeline/ipe/camxipepipelinetitan480.cpp        \
    titan17x/camxifeabf34titan17x.cpp               \
    titan17x/camxifeawbbgstats14titan17x.cpp        \
    titan17x/camxifebfstats23titan17x.cpp           \
    titan17x/camxifebhiststats14titan17x.cpp        \
    titan17x/camxifebls12titan17x.cpp               \
    titan17x/camxifebpcbcc50titan17x.cpp            \
    titan17x/camxifecamiftitan17x.cpp               \
    titan17x/camxifecamiflitetitan17x.cpp           \
    titan17x/camxifecc12titan17x.cpp                \
    titan17x/camxifecrop10titan17x.cpp              \
    titan17x/camxifecst12titan17x.cpp               \
    titan17x/camxifecsstats14titan17x.cpp           \
    titan17x/camxifedemosaic36titan17x.cpp          \
    titan17x/camxifedemosaic37titan17x.cpp          \
    titan17x/camxifedemux13titan17x.cpp             \
    titan17x/camxifeds410titan17x.cpp               \
    titan17x/camxifedualpd10titan17x.cpp            \
    titan17x/camxifegamma16titan17x.cpp             \
    titan17x/camxifegtm10titan17x.cpp               \
    titan17x/camxifehdr20titan17x.cpp               \
    titan17x/camxifehdr22titan17x.cpp               \
    titan17x/camxifehdr23titan17x.cpp               \
    titan17x/camxifehdrbestats15titan17x.cpp        \
    titan17x/camxifehdrbhiststats13titan17x.cpp     \
    titan17x/camxifeihiststats12titan17x.cpp        \
    titan17x/camxifelinearization33titan17x.cpp     \
    titan17x/camxifelsc34titan17x.cpp               \
    titan17x/camxifemnds16titan17x.cpp              \
    titan17x/camxifepdpc11titan17x.cpp              \
    titan17x/camxifepedestal13titan17x.cpp          \
    titan17x/camxifeprecrop10titan17x.cpp           \
    titan17x/camxifer2pd10titan17x.cpp              \
    titan17x/camxiferoundclamp11titan17x.cpp        \
    titan17x/camxifersstats14titan17x.cpp           \
    titan17x/camxifewb12titan17x.cpp                \
    titan17x/camxifetintlessbgstats15titan17x.cpp   \
    titan17x/bps/camxbpsdemosaic36titan17x.cpp      \
    titan17x/bps/camxbpsdemux13titan17x.cpp         \
    titan17x/bps/camxbpspedestal13titan17x.cpp      \
    titan17x/bps/camxbpshdr22titan17x.cpp           \
    titan17x/bps/camxbpscst12titan17x.cpp           \
    titan17x/bps/camxbpscc13titan17x.cpp            \
    titan17x/bps/camxbpsabf40titan17x.cpp           \
    titan17x/bps/camxbpsgic30titan17x.cpp           \
    titan17x/bps/camxbpshnr10titan17x.cpp           \
    titan17x/bps/camxbpsgtm10titan17x.cpp           \
    titan17x/bps/camxbpswb13titan17x.cpp            \
    titan17x/bps/camxbpslsc34titan17x.cpp           \
    titan17x/bps/camxbpslinearization34titan17x.cpp \
    titan17x/bps/camxbpsgamma16titan17x.cpp         \
    titan17x/bps/camxbpsbpcpdpc20titan17x.cpp       \
    titan17x/camxcvpica20titan17x.cpp               \
    titan17x/camxipe2dlut10titan17x.cpp             \
    titan17x/camxipeanr10titan17x.cpp               \
    titan17x/camxipeasf30titan17x.cpp               \
    titan17x/camxipecac22titan17x.cpp               \
    titan17x/camxipecc13titan17x.cpp                \
    titan17x/camxipechromaenhancement12titan17x.cpp \
    titan17x/camxipecs20titan17x.cpp                \
    titan17x/camxipecst12titan17x.cpp               \
    titan17x/camxipegamma15titan17x.cpp             \
    titan17x/camxipegra10titan17x.cpp               \
    titan17x/camxipeica10titan17x.cpp               \
    titan17x/camxipeica20titan17x.cpp               \
    titan17x/camxipeltm13titan17x.cpp               \
    titan17x/camxipesce11titan17x.cpp               \
    titan17x/camxipetf10titan17x.cpp               \
    titan17x/camxipeupscaler12titan17x.cpp          \
    titan17x/camxipeupscaler20titan17x.cpp          \
    titan480/bps/camxbpsabf40titan480.cpp           \
    titan480/bps/camxbpscc13titan480.cpp            \
    titan480/bps/camxbpscst12titan480.cpp           \
    titan480/bps/camxbpsdemux13titan480.cpp         \
    titan480/bps/camxbpsdemosaic36titan480.cpp      \
    titan480/bps/camxbpsgtm10titan480.cpp           \
    titan480/bps/camxbpsgic30titan480.cpp           \
    titan480/bps/camxbpsgamma16titan480.cpp         \
    titan480/bps/camxbpslinearization34titan480.cpp \
    titan480/bps/camxbpslsc40titan480.cpp           \
    titan480/bps/camxbpspdpc30titan480.cpp          \
    titan480/bps/camxbpspedestal13titan480.cpp      \
    titan480/bps/camxbpswb13titan480.cpp            \
    titan480/bps/camxbpshdr30titan480.cpp           \
    titan480/camxifebls12titan480.cpp               \
    titan480/camxifebfstats25titan480.cpp           \
    titan480/camxifecamiflcrtitan480.cpp            \
    titan480/camxifecamifpdaftitan480.cpp           \
    titan480/camxifecamifpptitan480.cpp             \
    titan480/camxifecamifrdititan480.cpp            \
    titan480/camxifeabf40titan480.cpp               \
    titan480/camxifeawbbgstats15titan480.cpp        \
    titan480/camxifecc13titan480.cpp                \
    titan480/camxifecrop11titan480.cpp              \
    titan480/camxifecsidrdititan480.cpp             \
    titan480/camxifecst12titan480.cpp               \
    titan480/camxifedemosaic36titan480.cpp          \
    titan480/camxifedemux13titan480.cpp             \
    titan480/camxifeds411titan480.cpp               \
    titan480/camxifedsx10titan480.cpp               \
    titan480/camxifegamma16titan480.cpp             \
    titan480/camxifegtm10titan480.cpp               \
    titan480/camxifehdr30titan480.cpp               \
    titan480/camxifehdrbestats15titan480.cpp        \
    titan480/camxifelinearization34titan480.cpp     \
    titan480/camxifelsc40titan480.cpp               \
    titan480/camxifemnds21titan480.cpp              \
    titan480/camxifelcr10titan480.cpp               \
    titan480/camxifepdaf20titan480.cpp              \
    titan480/camxifepdpc30titan480.cpp              \
    titan480/camxifepedestal13titan480.cpp          \
    titan480/camxifetintlessbgstats15titan480.cpp   \
    titan480/camxiferoundclamp11titan480.cpp        \
    titan480/camxifewb13titan480.cpp                \
    titan480/camxipe2dlut10titan480.cpp             \
    titan480/camxipeanr10titan480.cpp               \
    titan480/camxipeasf30titan480.cpp               \
    titan480/camxipecac23titan480.cpp               \
    titan480/camxipecc13titan480.cpp                \
    titan480/camxipechromaenhancement12titan480.cpp \
    titan480/camxipecs20titan480.cpp                \
    titan480/camxipecst12titan480.cpp               \
    titan480/camxipegamma15titan480.cpp             \
    titan480/camxipegra10titan480.cpp               \
    titan480/camxipehnr10titan480.cpp               \
    titan480/camxipeica30titan480.cpp               \
    titan480/camxipelenr10titan480.cpp              \
    titan480/camxipeltm14titan480.cpp               \
    titan480/camxipesce11titan480.cpp               \
    titan480/camxipetf20titan480.cpp                \
    titan480/camxipeupscaler20titan480.cpp          \
    titan480/camxifehdrbhiststats13titan480.cpp     \
    titan480/camxifebhiststats14titan480.cpp        \
    titan480/camxifeihiststats12titan480.cpp        \
    titan480/camxifersstats14titan480.cpp           \
    titan480/camxifecsstats14titan480.cpp

LOCAL_INC_FILES :=                                \
    camxisphwsetting.h                            \
    pipeline/camxisppipeline.h                    \
    pipeline/bps/camxbpspipelinetitan150.h        \
    pipeline/bps/camxbpspipelinetitan170.h        \
    pipeline/bps/camxbpspipelinetitan160.h        \
    pipeline/bps/camxbpspipelinetitan175.h        \
    pipeline/bps/camxbpspipelinetitan480.h        \
    pipeline/ife/camxtitan150ife.h                \
    pipeline/ife/camxtitan170ife.h                \
    pipeline/ife/camxtitan175ife.h                \
    pipeline/ife/camxtitan480ife.h                \
    pipeline/ipe/camxipepipelinetitan150.h        \
    pipeline/ipe/camxipepipelinetitan170.h        \
    pipeline/ipe/camxipepipelinetitan160.h        \
    pipeline/ipe/camxipepipelinetitan175.h        \
    pipeline/ipe/camxipepipelinetitan480.h        \
    titan17x/camxifeabf34titan17x.h               \
    titan17x/camxifeawbbgstats14titan17x.h        \
    titan17x/camxifebfstats23titan17x.h           \
    titan17x/camxifebls12titan17x.h               \
    titan17x/camxifebpcbcc50titan17x.h            \
    titan17x/camxifebhiststats14titan17x.h        \
    titan17x/camxifecamiftitan17x.h               \
    titan17x/camxifecamiflitetitan17x.h           \
    titan17x/camxifecc12titan17x.h                \
    titan17x/camxifecrop10titan17x.h              \
    titan17x/camxifecst12titan17x.h               \
    titan17x/camxifecsstats14titan17x.h           \
    titan17x/camxifedemosaic36titan17x.h          \
    titan17x/camxifedemosaic37titan17x.h          \
    titan17x/camxifedemux13titan17x.h             \
    titan17x/camxifeds410titan17x.h               \
    titan17x/camxifedualpd10titan17x.h            \
    titan17x/camxifegamma16titan17x.h             \
    titan17x/camxifegtm10titan17x.h               \
    titan17x/camxifehdr20titan17x.h               \
    titan17x/camxifehdr22titan17x.h               \
    titan17x/camxifehdr23titan17x.h               \
    titan17x/camxifehdrbestats15titan17x.h        \
    titan17x/camxifehdrbhiststats13titan17x.h     \
    titan17x/camxifeihiststats12titan17x.h        \
    titan17x/camxifelinearization33titan17x.h     \
    titan17x/camxifelsc34titan17x.h               \
    titan17x/camxifemnds16titan17x.h              \
    titan17x/camxifepdpc11titan17x.h              \
    titan17x/camxifepedestal13titan17x.h          \
    titan17x/camxifeprecrop10titan17x.h           \
    titan17x/camxifer2pd10titan17x.h              \
    titan17x/camxiferoundclamp11titan17x.h        \
    titan17x/camxifersstats14titan17x.h           \
    titan17x/camxifewb12titan17x.h                \
    titan17x/camxifetintlessbgstats15titan17x.h   \
    titan17x/bps/camxbpsdemosaic36titan17x.h      \
    titan17x/bps/camxbpsdemux13titan17x.h         \
    titan17x/bps/camxbpspedestal13titan17x.h      \
    titan17x/bps/camxbpshdr22titan17x.h           \
    titan17x/bps/camxbpscst12titan17x.h           \
    titan17x/bps/camxbpscc13titan17x.h            \
    titan17x/bps/camxbpsabf40titan17x.h           \
    titan17x/bps/camxbpsgic30titan17x.h           \
    titan17x/bps/camxbpshnr10titan17x.h           \
    titan17x/bps/camxbpsgtm10titan17x.h           \
    titan17x/bps/camxbpswb13titan17x.h            \
    titan17x/bps/camxbpslsc34titan17x.h           \
    titan17x/bps/camxbpslinearization34titan17x.h \
    titan17x/bps/camxbpsgamma16titan17x.h         \
    titan17x/bps/camxbpsbpcpdpc20titan17x.h       \
    titan17x/camxcvpica20titan17x.h               \
    titan17x/camxipe2dlut10titan17x.h             \
    titan17x/camxipeanr10titan17x.h               \
    titan17x/camxipeasf30titan17x.h               \
    titan17x/camxipecac22titan17x.h               \
    titan17x/camxipecc13titan17x.h                \
    titan17x/camxipechromaenhancement12titan17x.h \
    titan17x/camxipecs20titan17x.h                \
    titan17x/camxipecst12titan17x.h               \
    titan17x/camxipegamma15titan17x.h             \
    titan17x/camxipegra10titan17x.h               \
    titan17x/camxipeica10titan17x.h               \
    titan17x/camxipeica20titan17x.h               \
    titan17x/camxipeltm13titan17x.h               \
    titan17x/camxipesce11titan17x.h               \
    titan17x/camxipetf10titan17x.h                \
    titan17x/camxipeupscaler12titan17x.h          \
    titan17x/camxipeupscaler20titan17x.h          \
    titan480/bps/camxbpsabf40titan480.h           \
    titan480/bps/camxbpscc13titan480.h            \
    titan480/bps/camxbpscst12titan480.h           \
    titan480/bps/camxbpsdemux13titan480.h         \
    titan480/bps/camxbpsdemosaic36titan480.h      \
    titan480/bps/camxbpsgtm10titan480.h           \
    titan480/bps/camxbpsgic30titan480.h           \
    titan480/bps/camxbpsgamma16titan480.h         \
    titan480/bps/camxbpslinearization34titan480.h \
    titan480/bps/camxbpslsc40titan480.h           \
    titan480/bps/camxbpspdpc30titan480.h          \
    titan480/bps/camxbpspedestal13titan480.h      \
    titan480/bps/camxbpswb13titan480.h            \
    titan480/bps/camxbpshdr30titan480.h           \
    titan480/camxifeabf40titan480.h               \
    titan480/camxifeawbbgstats15titan480.h        \
    titan480/camxifebls12titan480.h               \
    titan480/camxifebfstats25titan480.h           \
    titan480/camxifecamiflcrtitan480.h            \
    titan480/camxifecamifpdaftitan480.h           \
    titan480/camxifecamifpptitan480.h             \
    titan480/camxifecamifrdititan480.h            \
    titan480/camxifecc13titan480.h                \
    titan480/camxifecrop11titan480.h              \
    titan480/camxifecsidrdititan480.h             \
    titan480/camxifecst12titan480.h               \
    titan480/camxifedemosaic36titan480.h          \
    titan480/camxifedemux13titan480.h             \
    titan480/camxifeds411titan480.h               \
    titan480/camxifedsx10titan480.h               \
    titan480/camxifegamma16titan480.h             \
    titan480/camxifegtm10titan480.h               \
    titan480/camxifehdr30titan480.h               \
    titan480/camxifehdrbestats15titan480.h        \
    titan480/camxifelinearization34titan480.h     \
    titan480/camxifelsc40titan480.h               \
    titan480/camxifemnds21titan480.h              \
    titan480/camxifelcr10titan480.h               \
    titan480/camxifepdaf20titan480.h              \
    titan480/camxifepdpc30titan480.h              \
    titan480/camxifepedestal13titan480.h          \
    titan480/camxifetintlessbgstats15titan480.h   \
    titan480/camxiferoundclamp11titan480.h        \
    titan480/camxifewb13titan480.h                \
    titan480/camxipe2dlut10titan480.h             \
    titan480/camxipeanr10titan480.h               \
    titan480/camxipeasf30titan480.h               \
    titan480/camxipecac23titan480.h               \
    titan480/camxipecc13titan480.h                \
    titan480/camxipechromaenhancement12titan480.h \
    titan480/camxipecs20titan480.h                \
    titan480/camxipecst12titan480.h               \
    titan480/camxipegamma15titan480.h             \
    titan480/camxipegra10titan480.h               \
    titan480/camxipeica30titan480.h               \
    titan480/camxipehnr10titan480.h               \
    titan480/camxipelenr10titan480.h              \
    titan480/camxipeltm14titan480.h               \
    titan480/camxipesce11titan480.h               \
    titan480/camxipetf20titan480.h                \
    titan480/camxipeupscaler20titan480.h          \
    titan480/camxifehdrbhiststats13titan480.h     \
    titan480/camxifebhiststats14titan480.h        \
    titan480/camxifeihiststats12titan480.h        \
    titan480/camxifersstats14titan480.h           \
    titan480/camxifecsstats14titan480.h

# Put here any libraries that should be linked by CAMX projects
LOCAL_C_LIBS := $(CAMX_C_LIBS)

# Paths to included headers
LOCAL_C_INCLUDES := $(CAMX_C_INCLUDES)                                \
    $(CAMX_PATH)/src/csl                                              \
    $(CAMX_PATH)/src/hwl/ife                                          \
    $(CAMX_PATH)/src/hwl/ipe                                          \
    $(CAMX_PATH)/src/hwl/bps                                          \
    $(CAMX_PATH)/src/hwl/dspinterfaces                                \
    $(CAMX_PATH)/src/hwl/iqsetting                                    \
    $(CAMX_PATH)/src/hwl/isphwsetting                                 \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan170                        \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan480                        \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan17x                        \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan17x/bps                    \
    $(CAMX_PATH)/src/hwl/isphwsetting/titan480/bps                    \
    $(CAMX_PATH)/src/hwl/ispiqmodule                                  \
    $(CAMX_PATH)/src/hwl/titan17x                                     \
    $(CAMX_PATH)/src/hwl/isphwsetting/pipeline                        \
    $(CAMX_OUT_HEADERS)/titan17x                                      \
    $(CAMX_OUT_HEADERS)/titan48x                                      \
    $(CAMX_OUT_HEADERS)                                               \

ifeq ($(IQSETTING),OEM1)
LOCAL_C_INCLUDES +=                                                   \
    $(CAMX_OEM1IQ_PATH)/iqsetting
else
LOCAL_C_INCLUDES +=                                                   \
    $(CAMX_PATH)/src/hwl/iqinterpolation
endif

LOCAL_C_INCLUDES +=                                                   \
    $(CAMX_CHICDK_API_PATH)/generated/g_chromatix

# Compiler flags
LOCAL_CFLAGS := $(CAMX_CFLAGS)
LOCAL_CPPFLAGS := $(CAMX_CPPFLAGS)

# Binary name
LOCAL_MODULE := libcamxisphwsetting

include $(CAMX_BUILD_STATIC_LIBRARY)
-include $(CAMX_CHECK_WHINER)
