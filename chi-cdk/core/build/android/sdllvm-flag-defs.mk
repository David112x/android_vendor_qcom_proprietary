# sdllvm-flag-defs.mk - Makefile to include SDLLVM optimization
#
# Include the optimization flags for SDLLVM compiler
#

# SDLLVM C flags
SDCLANG_FLAGS :=                                    \
    -ffp-contract=fast                              \
    -mcpu=cortex-a53                                \
    -mfpu=crypto-neon-fp-armv8                      \
    -Wno-logical-not-parentheses                    \
    -Wl,--no-fatal-warnings                         \
    -Wno-address-of-packed-member                   \
    -Wno-tautological-constant-out-of-range-compare

# Note: The Wno-x flags would need to be removed eventually once
# those warnings in the source are addressed.

ifeq ($(LOCAL_SDCLANG_OFAST), true)
  # no-fast-math must apear after Ofast
  SDCLANG_FLAGS += -Ofast -fno-fast-math
endif

# Append Android build top if path is not absolute.
ifneq ($(SDCLANG_PATH),$(filter /%,$(SDCLANG_PATH)))
  CAMERA_SDCLANG_ABS_PATH := ./$(SDCLANG_PATH)
else
  CAMERA_SDCLANG_ABS_PATH := $(SDCLANG_PATH)
endif

# Use community linker if SDLLVM version is greater than or equal 8.0
ifneq ($(wildcard $(CAMERA_SDCLANG_ABS_PATH)),)
  CAMERA_SDCLANG_VERSION := $(shell $(CAMERA_SDCLANG_ABS_PATH)/llvm-config --version)
  VERSION_MAJOR := $(shell echo $(CAMERA_SDCLANG_VERSION) | awk -F . '{print $$1}')
  ifneq ($(shell expr $(VERSION_MAJOR) \>= 8), 1)
    # Use SDLLVM linker only if version is less than 8.0
    SDCLANG_LINK := -fuse-ld=qcld
  else
    SDCLANG_LINK := -fuse-ld=lld
  endif
else
  # Use SDLLVM linker in general for all LLVM versions
  SDCLANG_LINK := -fuse-ld=qcld
endif

# Turn on LTO for libs which set LOCAL_SDCLANG_LTO.
# Enable Thin-LTO for versions greater than or equal to 8.0
SDCLANG_LTO  :=
ifeq ($(LOCAL_SDCLANG_LTO), true)
  SDCLANG_LTO := -flto
  ifneq ($(wildcard $(CAMERA_SDCLANG_ABS_PATH)),)
    CAMERA_SDCLANG_VERSION := $(shell $(CAMERA_SDCLANG_ABS_PATH)/llvm-config --version)
    VERSION_MAJOR := $(shell echo $(CAMERA_SDCLANG_VERSION) | awk -F . '{print $$1}')
    ifneq ($(shell expr $(VERSION_MAJOR) \>= 8), 1)
      SDCLANG_LTO := -flto
    else
      SDCLANG_LTO := -flto=thin -fuse-ld=lld
    endif
  endif
endif

# Note: LOCAL_SDCLANG_EXTRA_FLAGS can be set in the individual module's .mk
# file in order to override the default SDCLANG_FLAGS.

SDCLANG_CFLAGS   := $(SDCLANG_FLAGS) $(SDCLANG_LTO) $(SDCLANG_LINK)
SDCLANG_LDFLAGS  := $(SDCLANG_FLAGS) $(SDCLANG_LTO)

LOCAL_CFLAGS     += $(SDCLANG_CFLAGS)
LOCAL_CXX_FLAGS  += $(SDCLANG_CFLAGS)
LOCAL_LDFLAGS    += $(SDCLANG_LDFLAGS)
LOCAL_CFLAGS_32  += $(LOCAL_SDCLANG_EXTRA_FLAGS_32)
LOCAL_LDFLAGS_32 += $(LOCAL_SDCLANG_EXTRA_FLAGS_32)

