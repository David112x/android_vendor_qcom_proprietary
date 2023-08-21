TARGET_DISABLE_QTI_VPP := false

ifeq ($(call is-board-platform,$(TRINKET)),true)
    TARGET_DISABLE_QTI_VPP := true
endif
