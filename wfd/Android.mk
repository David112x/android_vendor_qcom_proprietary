ifeq ($(call is-vendor-board-platform,QCOM),true)

QMAA_DISABLES_WFD := false
ifeq ($(TARGET_USES_QMAA),true)
ifneq ($(TARGET_USES_QMAA_OVERRIDE_WFD),true)
QMAA_DISABLES_WFD := true
endif #TARGET_USES_QMAA_OVERRIDE_WFD
endif #TARGET_USES_QMAA

WFD_DISABLE_PLATFORM_LIST := msm8610 mpq8092 msm_bronze msm8909 msm8952

#Disable for selected 32-bit targets
ifeq ($(call is-board-platform,bengal),true)
ifeq ($(TARGET_BOARD_SUFFIX),_32)
WFD_DISABLE_PLATFORM_LIST += bengal
endif
endif

# ------------------------------------------------------------------------------
# Guard compilation of vendor projects (in intf) from qssi/qssi_32 as these modules
# would be scanned during initial stages of qssi/qssi_32 compilation.
# non-src shippable deps of such such modules would raise compilation issues
# in qssi/qssi_32 customer variant compilation.
ifneq ($(call is-product-in-list, qssi qssi_32),true)
ifneq ($(call is-board-platform-in-list,$(WFD_DISABLE_PLATFORM_LIST)),true)
ifneq ($(TARGET_HAS_LOW_RAM), true)
ifneq ($(QMAA_DISABLES_WFD),true)

LOCAL_PATH := $(call my-dir)

include $(call all-makefiles-under, $(LOCAL_PATH))
endif #QMAA_DISABLES_WFD
endif #TARGET_HAS_LOW_RAM
endif #DISABLE_LIST
endif #QSSI
endif # TARGET_USES_WFD
