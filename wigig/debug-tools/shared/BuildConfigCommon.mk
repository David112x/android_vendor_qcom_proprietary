PLATFORM_NAME := Unknown

PTHREAD_LINK_FLAGS := -pthread

LIBNL_INCLUDE_FLAGS :=
LIBNL_LINK_FLAGS :=

ifneq ($(CONFIG_TARGET_ipq)$(CONFIG_TARGET_ipq806x)$(CONFIG_TARGET_ipq807x)$(CONFIG_TARGET_ipq807x_64),)
    is_ipq = 1
endif

ifeq ($(WIGIG_3PP_BUILD), TRUE)
    # Third-party build
    PLATFORM_NAME := 3PP
    CROSS := $(TARGET_CROSS)
    CFLAGS := --sysroot=$(SYSROOT_CPP) $(CFLAGS)
    CPPFLAGS := --sysroot=$(SYSROOT_CPP) $(CPPFLAGS)
    LIBNL_INCLUDE_FLAGS += -I$(STAGING_DIR_PLATFORM)/usr/include/libnl3
    LDFLAGS += --sysroot=$(SYSROOT_CPP) -L$(STAGING_DIR_PLATFORM)/usr/lib
else ifeq ($(is_ipq), 1)
    ifneq ($(strip $(TOOLPREFIX)),)
        CROSS:=$(TOOLPREFIX)
    endif
    # OpenWrt

    PLATFORM_NAME := $(subst $\",,$(CONFIG_TARGET_SUBTARGET))
    ifeq ($(PLATFORM_NAME),)
        PLATFORM_NAME := $(subst $\",,$(CONFIG_TARGET_BOARD))
    endif
    ifeq ($(PLATFORM_NAME),)
        PLATFORM_NAME := ipq
    endif

    LIBNL_INCLUDE_FLAGS += $(shell pkg-config --cflags libnl-3.0)
    LIBNL_INCLUDE_FLAGS += $(shell pkg-config --cflags libnl-genl-3.0)
    LDFLAGS += -s
else ifneq (,$(findstring SUSE,$(shell lsb_release -a)))
    # RTL Simulation
    -include SUSE/SUSE.mk
    PLATFORM_NAME := RTL_SIM
else ifneq ($(ARM32_BUILD),)
    $(info ***** Build ARM32 static executable)
    CROSS := arm-linux-gnueabi-
    LIBNL_INCLUDE_FLAGS += -I../../libnl_headers_and_libs/include/libnl3
    LDFLAGS += --static -s
    LIBNL_LINK_FLAGS := -L../../libnl_headers_and_libs/lib
    PTHREAD_LINK_FLAGS := -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
    PLATFORM_NAME := ARM32
else
    # Ubuntu
    PLATFORM_NAME := x86
    LIBNL_INCLUDE_FLAGS += -I/usr/include/libnl3
    
    ifeq ($(UB14_BUILD), TRUE)
        # Ubuntu 14
        # assume TARGET_LIBS is set by the build system to the absolute path
        # of directory that contains all required libraries
        TOOLS_GCC_CUSTOM_VERSION := -4.8
        LDFLAGS += -Wl,-rpath,$(TARGET_LIBS)
    endif
endif

CC := $(CROSS)gcc$(TOOLS_GCC_CUSTOM_VERSION)
CXX := $(CROSS)g++$(TOOLS_GCC_CUSTOM_VERSION)

$(info ***** PLATFORM_NAME=$(PLATFORM_NAME) *****)

CPPFLAGS += -Wall -Wextra -g -MMD -std=c++0x -fPIE -DPLATFORM_NAME=$(PLATFORM_NAME)
CFLAGS += -Wall -Wextra -g -MMD -fPIE -DPLATFORM_NAME=$(PLATFORM_NAME)
LDFLAGS += -fPIE -pie
