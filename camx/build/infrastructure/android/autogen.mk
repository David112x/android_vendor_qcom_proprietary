ifneq ($(CAMX_AUTOGEN_MK_INCLUDED),1)
CAMX_AUTOGEN_MK_INCLUDED := 1

LOCAL_PATH := $(abspath $(call my-dir)/../../..)

ifeq ($(CAMX_PATH),)
CAMX_PATH := $(LOCAL_PATH)
endif

CAMX_BUILD_PATH := $(CAMX_PATH)/build/infrastructure/android

include $(CLEAR_VARS)

# Get definitions common to the CAMX project here
include $(CAMX_BUILD_PATH)/common.mk

# Auto-generated files
# Note: using if check for xml file because this can get run in other directories
CAMX_TOOLS_PATH := $(CAMX_BUILD_PATH)/../tools

CAMX_TXTOUT_PATH=$(CAMX_BUILD_PATH)/built/settings

# Note: generating all settings files if any dependency changes to ensure we generate a complete camxoverridesettings.txt
CAMX_OVERRIDESETTINGS_PATH := $(CAMX_PATH)/build/infrastructure/android/built/settings/camxoverridesettings.txt
ifeq ($CAMX_EXT_VBUILD, 1)
    CAMX_OVERRIDESETTINGS_PATH := $(CAMX_PATH)/internal_tools/settings/camxoverridesettings.txt
    ifeq ($CAMX_EXT_LNDKBUILD, 1)
        CAMX_OVERRIDESETTINGS_PATH := $(CAMX_PATH)/build/infrastructure/android/built/settings/camxoverridesettings.txt
    endif
endif

 # SETTINGS COMPOSER definitions
COMPOSER_INPUT         := $(CAMX_PATH)/src/settings/common/camxsettings.xml
COMPOSER_OUTPUT        := $(CAMX_PATH)/src/settings/g_camxsettings.xml
COMPOSER_TARGET_INPUT  := $(CAMX_PATH)/src/settings/$(TARGET_BOARD_PLATFORM)/camxsettings.xml
COPYRIGHT_HEADER       := $(CAMX_CHICDK_PATH)/tools/usecaseconverter/xmlheader.txt
CAMXSETTINGSCOMPOSER   := python $(CAMX_PATH)/src/settings/settingscomposer.py

COMPOSER_CMD           := $(info $(shell ($(CAMXSETTINGSCOMPOSER) -c $(COMPOSER_INPUT) -t $(COMPOSER_TARGET_INPUT) -o $(COMPOSER_OUTPUT) -H $(COPYRIGHT_HEADER))))

# Run SETTINGS COMPOSER to generate g_camxsettings.xml
COMPOSER_STATUS := $(shell $(COMPOSER_CMD))
$(info $(COMPOSER_STATUS))

# The ordering of the XML input files below is determined by the settingsgenerator.pl script
# which expected common settings files first, then hardware-specific settings files last.
CAMX_SETTINGS_INPUTS =                                          \
    $(CAMX_PATH)/src/settings/g_camxsettings.xml                \
    $(CAMX_PATH)/src/hwl/titan17x/camxtitan17xsettings.xml

# The ordering of the XML output files below is determined by the settingsgenerator.pl script
# which expected common settings files first, then hardware-specific settings files last.
CAMX_SETTINGS_OUTPUTS =                                         \
    $(CAMX_PATH)/src/settings/g_camxsettings.cpp                \
    $(CAMX_PATH)/src/settings/g_camxsettings.h                  \
    $(CAMX_PATH)/src/hwl/titan17x/g_camxtitan17xsettings.cpp    \
    $(CAMX_PATH)/src/hwl/titan17x/g_camxtitan17xsettings.h      \
    $(CAMX_OVERRIDESETTINGS_PATH)

CAMX_VERSION_OUTPUT = \
    $(CAMX_PATH)/src/core/g_camxversion.h

CAMX_PROPS_INPUT = \
    $(CAMX_PATH)/src/core/camxproperties.xml

CAMX_PROPS_OUTPUT = \
    $(CAMX_PATH)/src/core/g_camxproperties

CAMXSETTINGSGENERATORTOOL := perl $(CAMX_TOOLS_PATH)/settingsgenerator/settingsgenerator.pl

CAMXVERSIONTOOL := perl $(CAMX_TOOLS_PATH)/version.pl

CAMXPROPSTOOL := perl $(CAMX_TOOLS_PATH)/props.pl

$(info $(shell $(CAMXSETTINGSGENERATORTOOL) $(CAMX_SETTINGS_INPUTS) $(CAMX_OVERRIDESETTINGS_PATH)))
$(info $(shell $(CAMXVERSIONTOOL) $(CAMX_VERSION_OUTPUT)))
$(info $(shell $(CAMXPROPSTOOL) $(CAMX_PROPS_INPUT) $(CAMX_PROPS_OUTPUT)))

$(COMPOSER_OUTPUT) : .KATI_IMPLICIT_OUTPUTS := $(COMPOSER_INPUT)

$(CAMX_SETTINGS_OUTPUTS) : .KATI_IMPLICIT_OUTPUTS += $(CAMX_SETTINGS_INPUTS) $(CAMX_TXTOUT_PATH)/camxoverridesettings.txt

$(CAMX_SCOPE_OUTPUTS) : .KATI_IMPLICIT_OUTPUTS += $(CAMX_SCOPE_INPUTS)

$(CAMX_VERSION_OUTPUT) : .KATI_IMPLICIT_OUTPUTS += $(CAMX_TOOLS_PATH)/version.pl

endif # CAMX_AUTOGEN_MK_INCLUDED
