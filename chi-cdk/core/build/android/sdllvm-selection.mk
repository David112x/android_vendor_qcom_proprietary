# sdllvm-selection.mk
#
# Decide which version of SDLLVM to use depends on the availability
# and update the LOCAL_SDCLANG and LOCAL_SDCALNG_2
#

# Use SDCLANG if its version is >= 4.
# Else use SDCLANG_2 if it exists and its version is >= 4.
# Else use SDCLANG.

# Append Android build top if path is not absolute.
ifneq ($(SDCLANG_PATH),$(filter /%,$(SDCLANG_PATH)))
  CAMERA_SDCLANG_ABS_PATH := ./$(SDCLANG_PATH)
else
  CAMERA_SDCLANG_ABS_PATH := $(SDCLANG_PATH)
endif

ifneq ($(wildcard $(CAMERA_SDCLANG_ABS_PATH)),)
  CAMERA_SDCLANG_VERSION := $(shell $(CAMERA_SDCLANG_ABS_PATH)/llvm-config --version)
  VERSION_MAJOR := $(shell echo $(CAMERA_SDCLANG_VERSION) | awk -F . '{print $$1}')
else
  # CAMERA_SDCLANG_ABS_PATH does not exist.
  CAMERA_USE_SDCLANG := false
endif

ifeq ($(CAMERA_USE_SDCLANG),)
  CAMERA_USE_SDCLANG   := true
  CAMERA_USE_SDCLANG_2 := false

  ifneq ($(shell expr $(VERSION_MAJOR) \>= 4), 1)
    # Append Android build top if path is not absolute.
    ifneq ($(SDCLANG_PATH_2),$(filter /%,$(SDCLANG_PATH_2)))
      CAMERA_SDCLANG_ABS_PATH_2 := $(ANDROID_BUILD_TOP)/$(SDCLANG_PATH_2)
    else
      CAMERA_SDCLANG_ABS_PATH_2 := $(SDCLANG_PATH_2)
    endif

    ifneq ($(wildcard $(CAMERA_SDCLANG_ABS_PATH_2)),)
      CAMERA_SDCLANG_VERSION_2 := $(shell $(CAMERA_SDCLANG_ABS_PATH_2)/llvm-config --version)
      VERSION_MAJOR_2 := $(shell echo $(CAMERA_SDCLANG_VERSION_2) | awk -F . '{print $$1}')
      ifeq ($(shell expr $(VERSION_MAJOR_2) \>= 4), 1)
        CAMERA_USE_SDCLANG  := false
        CAMERA_USE_SDCLANG_2 := true
      endif
    else
      # CAMERA_SDCLANG_ABS_PATH_2 does not exist.
      CAMERA_USE_SDCLANG_2 := false
    endif
  endif
endif

LOCAL_SDCLANG     := $(CAMERA_USE_SDCLANG)
LOCAL_SDCLANG_2   := $(CAMERA_USE_SDCLANG_2)
LOCAL_SDCLANG_LTO := true

# SDCLANG does not support CLANG_LLD. Disable CLANG_LLD when
# SDCLANG is in use and its version is less than 8.

# With SDLLVM 10 LLD should be used and for versions less than 10, LLD should be disabled.
ifeq ($(LOCAL_SDCLANG),true)
  ifneq ($(shell expr $(VERSION_MAJOR) \>= 8), 1)
    LOCAL_USE_CLANG_LLD := false
  else
    LOCAL_USE_CLANG_LLD := true
  endif
endif
ifeq ($(LOCAL_SDCLANG_2),true)
  ifneq ($(shell expr $(VERSION_MAJOR) \>= 8), 1)
    LOCAL_USE_CLANG_LLD := false
  else
    LOCAL_USE_CLANG_LLD := true
  endif
endif
