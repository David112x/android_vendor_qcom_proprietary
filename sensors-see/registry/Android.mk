LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# List of sensors filenames
sns_config_files := $(wildcard $(LOCAL_PATH)/config/common/*.json)
sns_config_files += $(wildcard $(LOCAL_PATH)/config/$(TARGET_BOARD_PLATFORM)/*.json)

ifeq ($(TARGET_PRODUCT), kona)
sns_config_files += $(wildcard $(LOCAL_PATH)/config/sxr2130/*.json)
endif

ifneq ($(call is-board-platform-in-list,msmnile $(MSMSTEPPE) kona sdm845 sdm710),true)

$(info "Adding sensors config files to persist.img  = $(sns_config_files)")
define _add-config-files
 include $$(CLEAR_VARS)
 LOCAL_MODULE := $(notdir $(1))
 LOCAL_MODULE_STEM := $(notdir $(1))
 sns_config_modules += $$(LOCAL_MODULE)
 LOCAL_SRC_FILES := $(1:$(LOCAL_PATH)/%=%)
 LOCAL_MODULE_TAGS := optional
 LOCAL_MODULE_CLASS := TEMP
 LOCAL_MODULE_PATH := $$(PRODUCT_OUT)/persist/sensors/registry/config
include $$(BUILD_PREBUILT)
endef

sns_config_modules :=
sns_config :=
$(foreach _conf-file, $(sns_config_files), $(eval $(call _add-config-files,$(_conf-file))))
endif

$(info "Adding sensors config files to persist.img  = $(sns_config_files)")
define _add-etc-config-files
 include $$(CLEAR_VARS)
 LOCAL_MODULE := $(notdir $(1))
 LOCAL_MODULE_STEM := $(notdir $(1))
 sns_config_modules += $$(LOCAL_MODULE)
 LOCAL_SRC_FILES := $(1:$(LOCAL_PATH)/%=%)
 LOCAL_MODULE_TAGS := optional
 LOCAL_MODULE_CLASS := ETC
 LOCAL_MODULE_PATH := $$(TARGET_OUT_VENDOR)/etc/sensors/config
include $$(BUILD_PREBUILT)
endef

sns_config_modules :=
sns_config :=
$(foreach _conf-file, $(sns_config_files), $(eval $(call _add-etc-config-files,$(_conf-file))))

include $(CLEAR_VARS)
LOCAL_MODULE := sensors_config_module
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(sns_config_modules)
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := sensors_registry
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/persist/sensors/registry/registry
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := sensors_settings
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/persist/sensors
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifneq ($(call is-board-platform-in-list,msmnile $(MSMSTEPPE) kona),true)
include $(CLEAR_VARS)
LOCAL_MODULE := sns_reg_config
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/persist/sensors/registry
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := sns_reg_version
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := qti
LOCAL_MODULE_CLASS = ETC
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/persist/sensors/registry
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

$(shell mkdir -p $(TARGET_OUT_VENDOR_ETC)/sensors)
$(shell cat $(LOCAL_PATH)/sns_reg_config > $(TARGET_OUT_VENDOR_ETC)/sensors/sns_reg_config)
