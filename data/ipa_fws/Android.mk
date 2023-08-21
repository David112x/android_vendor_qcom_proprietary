TARGET_LIST := msm8998 sdm845 sdm710 msmnile $(MSMSTEPPE) $(TRINKET) kona lito atoll bengal lahaina

GEN_IMG_DIR := $(TARGET_OUT_VENDOR)/firmware/
$(info $(shell mkdir -p $(GEN_IMG_DIR)))
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

PILSPLITTER_BASE := vendor/qcom/proprietary/common/scripts
HASH_SEG_ALGO := sha384

ifeq ($(call is-board-platform-in-list,$(TARGET_LIST)),true)

$(info IPA_FWS: Checking $(TARGET_BOARD_PLATFORM) target)

#Disable ipa_fws.rc script for hana gvmq target
ifeq ($(call is-board-platform,msmnile),true)
ifeq ($(TARGET_BOARD_SUFFIX),_gvmq)
IPA_FWS_DISABLE_PLATFORM_LIST := msmnile
endif
endif

# Firmware hardware addresses and bin files
# If these differ across devices, define them accordingly.
DPS_ADDR := 0x01E5E000
HPS_ADDR := 0x01E5E080
GSI_ADDR := 0x01E08000
HPS_BIN := hps.bin

GEN_IPA_UC := false

FW_FILE_NAME := ipa_fws
UC_FW_FILE_NAME := ipa_uc

# msm8998 IPA FWs configs (IPAv3.1)
ifeq ($(call is-board-platform-in-list,msm8998),true)
$(info IPA_FWS creation for msm8998)
GSI_FW_FILE := fw_mhi_ipa_v3.bin
GSI_MEM_SIZE := 0x4000
IPA_SOC_HW_VERSION := 0x30020000
# For 8997 HW version
IPA_SOC_VERS := 0x3005
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/common/scripts/SecImage
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev2.xml
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #msm8998 check

# sdm845 IPA FWs configs (IPAv3.5.1)
ifeq ($(call is-board-platform-in-list,sdm845),true)
$(info IPA_FWS creation for sdm845)
GSI_FW_FILE := fw_mhi_ipa_v3.5.bin
GSI_MEM_SIZE := 0x4000
IPA_SOC_HW_VERSION := 0x60000000
IPA_SOC_VERS := 0x6001
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev2.xml
SECIMAGE_BASE := vendor/qcom/proprietary/common/scripts/SecImage
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #sdm845 check

# sdm710 IPA FWs configs (IPAv3.5.1)
ifeq ($(call is-board-platform-in-list,sdm710),true)
$(info IPA_FWS creation for sdm710)
GSI_FW_FILE := fw_mhi_ipa_v3.5.bin
GSI_MEM_SIZE := 0x4000
IPA_SOC_HW_VERSION := 0x60040000
IPA_SOC_VERS := 0x6005 0x6009 0x600A
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev2.xml
SECIMAGE_BASE := vendor/qcom/proprietary/common/scripts/SecImage
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #sdm710 check

# msmnile IPA FWs configs (IPAv4.1)
ifeq ($(call is-board-platform-in-list,msmnile),true)
$(info IPA_FWS creation for msmnile)
GSI_FW_FILE := fw_mhi_ipa_v4.0.bin
GSI_MEM_SIZE := 0x6000
IPA_SOC_HW_VERSION := 0x60030100
IPA_SOC_VERS := 0x6003
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
GEN_IPA_UC := true
UC_FW_FILE := ipa_uc_v4.1.bin
UC_ADDR := 0x1E60000
UC_MEM_SIZE := 0x10000
include $(LOCAL_PATH)/uc_firmware_create.mk
endif #msmnile check

# MSMSTEPPE IPA FWs configs (IPAv4.2)
ifeq ($(call is-board-platform-in-list,$(MSMSTEPPE)),true)
$(info IPA_FWS creation for $(MSMSTEPPE))
GSI_FW_FILE := fw_mhi_ipa_v4.2.bin
GSI_MEM_SIZE := 0x4000
IPA_SOC_HW_VERSION := 0x60070000
IPA_SOC_VERS := "0x6007 0x600c"
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #MSMSTEPPE check

# atoll IPA FWs configs (IPAv4.2)
ifeq ($(call is-board-platform-in-list,atoll),true)
$(info IPA_FWS creation for atoll)
GSI_FW_FILE := fw_mhi_ipa_v4.2.bin
GSI_MEM_SIZE := 0x4000
IPA_SOC_HW_VERSION := 0x600E0100
IPA_SOC_VERS := "0x600E"
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #atoll check

# TRINKET IPA FWs configs (IPAv4.2)
ifeq ($(call is-board-platform-in-list,$(TRINKET)),true)
$(info IPA_FWS creation for $(TRINKET))
DPS_ADDR := 0x0585E000
HPS_ADDR := 0x0585E080
GSI_ADDR := 0x05808000
GSI_FW_FILE := fw_mhi_ipa_v4.2.bin
GSI_MEM_SIZE := 0x4000
IPA_SOC_HW_VERSION := 0x90010100
IPA_SOC_VERS := 0x9001
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #TRINKET check

# kona IPA FWs configs (IPAv4.5)
ifeq ($(call is-board-platform-in-list,kona),true)
$(info IPA_FWS creation for kona)
DPS_ADDR := 0x01E81000
HPS_ADDR := 0x01E81080
GSI_ADDR := 0x01E1F000
HPS_BIN := hps_v4.5.bin
GSI_FW_FILE := fw_mhi_ipa_v4.5.bin
GSI_MEM_SIZE := 0x8000
IPA_SOC_HW_VERSION := 0x60080200
IPA_SOC_VERS := 0x6008
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
GEN_IPA_UC := true
UC_FW_FILE := ipa_uc_v4.5.bin
UC_ADDR := 0x1EA0000
UC_MEM_SIZE := 0x10000
include $(LOCAL_PATH)/uc_firmware_create.mk
endif #kona check

# lito IPA FWs configs (IPAv4.7)
ifeq ($(call is-board-platform-in-list,lito),true)
$(info IPA_FWS creation for lito)
DPS_ADDR := 0x01E81000
HPS_ADDR := 0x01E81080
GSI_ADDR := 0x01E1F000
HPS_BIN := hps_v4.5.bin
GSI_FW_FILE := fw_mhi_ipa_v4.7.bin
GSI_MEM_SIZE := 0x5000
IPA_SOC_HW_VERSION := 0x600D0100
IPA_SOC_VERS := 0x600D
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
$(info IPA_FWs creation for bitra)
IPA_SOC_HW_VERSION := 0x60120100
IPA_SOC_VERS := 0x6012
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimage_eccv3.xml
FW_FILE_NAME := lagoon_ipa_fws
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #lito check

# Bengal IPA FWs configs (IPAv4.2)
ifeq ($(call is-board-platform-in-list,bengal),true)
$(info IPA_FWS creation for bengal)
DPS_ADDR := 0x0585E000
HPS_ADDR := 0x0585E080
GSI_ADDR := 0x05808000
HPS_BIN := hps.bin
GSI_FW_FILE := fw_mhi_ipa_v4.2.bin
GSI_MEM_SIZE := 0x4000
IPA_SOC_HW_VERSION := 0x90020100
IPA_SOC_VERS := 0x9002
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
$(info IPA_FWs creation for Agatti)
IPA_SOC_HW_VERSION := 0x90030100
IPA_SOC_VERS := 0x9003
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimage_eccv3.xml
FW_FILE_NAME := scuba_ipa_fws
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #Bengalcheck


# lahaina IPA FWs configs (IPAv4.9)
ifeq ($(call is-board-platform-in-list,lahaina),true)
$(info IPA_FWS creation for lahaina)
DPS_ADDR := 0x01E81000
HPS_ADDR := 0x01E81080
GSI_ADDR := 0x01E1F000
HPS_BIN := hps_v4.5.bin
GSI_FW_FILE := fw_mhi_ipa_v4.9.bin
GSI_MEM_SIZE := 0x6000
IPA_SOC_HW_VERSION := 0x600F0100
IPA_SOC_VERS := 0x600F
IPA_SECIMAGE_CONFIG_FILE := config/integration/secimagev3.xml
ifeq ("$(wildcard vendor/qcom/proprietary/sectools)","")
SECIMAGE_BASE := vendor/qcom/proprietary/tools/sectools
else
SECIMAGE_BASE := vendor/qcom/proprietary/sectools
endif
include $(LOCAL_PATH)/gsi_firmware_create.mk
endif #lahaina check

else #Target check
$(info IPA_FWS: unknow target $(TARGET_BOARD_PLATFORM). skipping)
endif #Target check
