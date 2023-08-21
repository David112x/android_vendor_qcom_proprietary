# Board config
ifneq ($(TARGET_BOARD_PLATFORM),bengal)
        OVERRIDE_RS_DRIVER := libRSDriver_adreno.so
endif


