# uC image creation
ifeq ($(GEN_IPA_UC),true)
$(info IPA_FWS: Create uC image files)

# Check uC FW file existence
ifeq ("$(wildcard $(LOCAL_PATH)/$(UC_FW_FILE))","")
$(info IPA_FWS: $(LOCAL_PATH)/$(UC_FW_FILE) does not exist!)
else # uC FW file existence check
BINS_UC := $(LOCAL_PATH)/$(UC_FW_FILE)
TOOLS_UC := $(LOCAL_PATH)/elf_creator_uc.py $(SECIMAGE_BASE)/sectools_builder.py $(PILSPLITTER_BASE)/pil-splitter.py
OUT_UC_FW := $(OUT_DIR)/ipa_tmp_uc_$(UC_FW_FILE_NAME)

GEN_IMG_UC := $(UC_FW_FILE_NAME).elf
ifeq ($(USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN),1)
GEN_IMG_UC_OTHERS := $(UC_FW_FILE_NAME).b00 $(UC_FW_FILE_NAME).b01 $(UC_FW_FILE_NAME).b02 $(UC_FW_FILE_NAME).mdt
GEN_IMG_UC_LAST := $(UC_FW_FILE_NAME).b02
else
GEN_IMG_UC_OTHERS := $(UC_FW_FILE_NAME).b00 $(UC_FW_FILE_NAME).mdt
GEN_IMG_GSI_LAST := $(UC_FW_FILE_NAME).b00
endif
$(info GEN_IMG_UC_OTHERS=$(GEN_IMG_UC_OTHERS) GEN_IMG_UC_LAST=$(GEN_IMG_UC_LAST))

include $(CLEAR_VARS)
LOCAL_MODULE := $(UC_FW_FILE_NAME)
LOCAL_SANITIZE:=integer_overflow
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_PATH := $(OUT_UC_FW)
LOCAL_REQUIRED_MODULES := $(GEN_IMG_UC) $(GEN_IMG_UC_OTHERS)
include $(BUILD_PHONY_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := $(UC_FW_FILE_NAME).b00
LOCAL_SANITIZE:=integer_overflow
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := ipa
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_PATH := $(GEN_IMG_DIR)
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): PRIVATE_FILE := $(LOCAL_MODULE)
$(LOCAL_BUILT_MODULE): PRIVATE_OUT_UC_FW := $(OUT_UC_FW)
$(LOCAL_BUILT_MODULE): $(GEN_IMG_DIR)/$(GEN_IMG_UC)
	echo "IPA_FWS staging $(PRIVATE_FILE)"
	if [ -d $(PRIVATE_OUT_UC_FW) ]; then \
		cp -f $(PRIVATE_OUT_UC_FW)/$(PRIVATE_FILE) $@; \
	else \
		touch $@; \
	fi

ifeq ($(USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN),1)
include $(CLEAR_VARS)
LOCAL_MODULE := $(UC_FW_FILE_NAME).b01
LOCAL_SANITIZE:=integer_overflow
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := ipa
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_PATH := $(GEN_IMG_DIR)
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): PRIVATE_FILE := $(LOCAL_MODULE)
$(LOCAL_BUILT_MODULE): PRIVATE_OUT_UC_FW := $(OUT_UC_FW)
$(LOCAL_BUILT_MODULE): $(GEN_IMG_DIR)/$(UC_FW_FILE_NAME).b00
	echo "IPA_FWS staging $(PRIVATE_FILE)"
	if [ -d $(PRIVATE_OUT_UC_FW) ]; then \
		cp -f $(PRIVATE_OUT_UC_FW)/$(PRIVATE_FILE) $@; \
	else \
		touch $@; \
	fi

include $(CLEAR_VARS)
LOCAL_MODULE := $(UC_FW_FILE_NAME).b02
LOCAL_SANITIZE:=integer_overflow
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := ipa
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_PATH := $(GEN_IMG_DIR)
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): PRIVATE_FILE := $(LOCAL_MODULE)
$(LOCAL_BUILT_MODULE): PRIVATE_OUT_UC_FW := $(OUT_UC_FW)
$(LOCAL_BUILT_MODULE): $(GEN_IMG_DIR)/$(UC_FW_FILE_NAME).b01
	echo "IPA_FWS staging $(PRIVATE_FILE)"
	if [ -d $(PRIVATE_OUT_UC_FW) ]; then \
		cp -f $(PRIVATE_OUT_UC_FW)/$(PRIVATE_FILE) $@; \
	else \
		touch $@; \
	fi
endif

include $(CLEAR_VARS)
LOCAL_MODULE := $(UC_FW_FILE_NAME).mdt
LOCAL_SANITIZE:=integer_overflow
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := ipa
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_PATH := $(GEN_IMG_DIR)
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): PRIVATE_FILE := $(LOCAL_MODULE)
$(LOCAL_BUILT_MODULE): PRIVATE_OUT_UC_FW := $(OUT_UC_FW)
$(LOCAL_BUILT_MODULE): $(GEN_IMG_DIR)/$(GEN_IMG_UC) $(GEN_IMG_DIR)/$(GEN_IMG_UC_LAST)
	echo "IPA_FWS staging $(PRIVATE_FILE)"
	if [ -d $(PRIVATE_OUT_UC_FW) ]; then \
		cp -f $(PRIVATE_OUT_UC_FW)/$(PRIVATE_FILE) $@; \
		rm -rf $(PRIVATE_OUT_UC_FW); \
	else \
		touch $@; \
	fi

include $(CLEAR_VARS)
LOCAL_MODULE := $(GEN_IMG_UC)
LOCAL_SANITIZE:=integer_overflow
LOCAL_MODULE_CLASS := FAKE
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := ipa
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
LOCAL_MODULE_PATH := $(GEN_IMG_DIR)
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): PRIVATE_PATH := $(LOCAL_PATH)
$(LOCAL_BUILT_MODULE): PRIVATE_ELF_FILE := $(LOCAL_MODULE)
$(LOCAL_BUILT_MODULE): PRIVATE_FW_FILE_NAME := $(UC_FW_FILE_NAME)
$(LOCAL_BUILT_MODULE): PRIVATE_UC_FW_FILE := $(UC_FW_FILE)
$(LOCAL_BUILT_MODULE): PRIVATE_UC_ADDR := $(UC_ADDR)
$(LOCAL_BUILT_MODULE): PRIVATE_UC_MEM_SIZE := $(UC_MEM_SIZE)
$(LOCAL_BUILT_MODULE): PRIVATE_SOC_VERS := $(IPA_SOC_VERS)
$(LOCAL_BUILT_MODULE): PRIVATE_SOC_HW_VERSION := $(IPA_SOC_HW_VERSION)
$(LOCAL_BUILT_MODULE): PRIVATE_SECIMAGE_CONFIG_FILE := $(IPA_SECIMAGE_CONFIG_FILE)
$(LOCAL_BUILT_MODULE): PRIVATE_OUT_UC_FW := $(OUT_UC_FW)
$(LOCAL_BUILT_MODULE): $(BINS_UC) $(TOOLS_UC)
	mkdir -p $(PRIVATE_OUT_UC_FW)
	# Assemble binaries/firmwares to a single ELF file
	python $(PRIVATE_PATH)/elf_creator_uc.py \
		--uc_fw $(PRIVATE_PATH)/$(PRIVATE_UC_FW_FILE) \
		--uc_fw_address $(PRIVATE_UC_ADDR) \
		--uc_fw_mem_size $(PRIVATE_UC_MEM_SIZE) \
		--outfile $(PRIVATE_OUT_UC_FW)/$(PRIVATE_ELF_FILE)
	echo IPA_FWS: Creating ipa ELF image OUT folders
	mkdir -p $(PRODUCT_OUT)/ipa
	mkdir -p $(PRODUCT_OUT)/ipa/signed
	mkdir -p $(PRODUCT_OUT)/ipa/unsigned
	echo IPA_FWS: install unsigned $(PRIVATE_ELF_FILE) at $(PRODUCT_OUT)/ipa/unsigned
	cp -f $(PRIVATE_OUT_UC_FW)/$(PRIVATE_ELF_FILE) $(PRODUCT_OUT)/ipa/unsigned
	# Sign the ELF file using SecImage tool
	SECIMAGE_LOCAL_DIR=$(SECIMAGE_BASE) USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN=$(USES_SEC_POLICY_MULTIPLE_DEFAULT_SIGN) \
	python $(SECIMAGE_BASE)/sectools_builder.py \
                -i $(PRIVATE_OUT_UC_FW)/$(PRIVATE_ELF_FILE) \
                -g ipa_uc \
                -t $(PRIVATE_OUT_UC_FW) \
                --install_base_dir=$(PRIVATE_OUT_UC_FW) \
                --config $(SECIMAGE_BASE)/$(PRIVATE_SECIMAGE_CONFIG_FILE) \
                --soc_hw_version=$(PRIVATE_SOC_HW_VERSION) \
                --soc_vers=$(PRIVATE_SOC_VERS) \
                > $(PRIVATE_OUT_UC_FW)/secimage.log 2>&1
	echo IPA_FWS: install signed $(PRIVATE_ELF_FILE) at $(PRODUCT_OUT)/ipa/signed
	cp -f $(PRIVATE_OUT_UC_FW)/$(PRIVATE_ELF_FILE) $(PRODUCT_OUT)/ipa/signed
	# PIL split the output of the SecImage tool
	python $(PILSPLITTER_BASE)/pil-splitter.py \
		$(PRIVATE_OUT_UC_FW)/$(PRIVATE_ELF_FILE) \
		$(PRIVATE_OUT_UC_FW)/$(PRIVATE_FW_FILE_NAME)
	cp -f $(PRIVATE_OUT_UC_FW)/$(PRIVATE_FW_FILE_NAME).elf $@
endif # uC FW file existence check

else # uC image creation
$(info IPA_FWS: Do not create uC image files)
endif # uC image creation

