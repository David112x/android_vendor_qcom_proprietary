ifeq ($(call is-board-platform-in-list, sdm845 sdm710 msmnile $(MSMSTEPPE) $(TRINKET) kona lito bengal atoll),true)
TARGET_USES_NQ_NFC := true
endif

#NQ NFC Config files + firmware images
NQ_VENDOR_NFC_PROP := nqnfcinfo
NQ_VENDOR_NFC_PROP += libnfc-mtp_rf1.conf
NQ_VENDOR_NFC_PROP += libnfc-mtp_rf2.conf
NQ_VENDOR_NFC_PROP += libnfc-mtp_default.conf
NQ_VENDOR_NFC_PROP += libnfc-mtp-NQ3XX.conf
NQ_VENDOR_NFC_PROP += libnfc-mtp-NQ4XX.conf
NQ_VENDOR_NFC_PROP += libnfc-mtp-SN100.conf
NQ_VENDOR_NFC_PROP += libnfc-mtp-SN100_38_4MHZ.conf
NQ_VENDOR_NFC_PROP += libnfc-qrd_rf1.conf
NQ_VENDOR_NFC_PROP += libnfc-qrd_rf2.conf
NQ_VENDOR_NFC_PROP += libnfc-qrd_default.conf
NQ_VENDOR_NFC_PROP += libnfc-qrd-NQ3XX.conf
NQ_VENDOR_NFC_PROP += libnfc-qrd-NQ4XX.conf
NQ_VENDOR_NFC_PROP += libnfc-qrd-SN100.conf
NQ_VENDOR_NFC_PROP += libnfc-qrd-SN100_38_4MHZ.conf
NQ_VENDOR_NFC_PROP += libpn551_fw.so
NQ_VENDOR_NFC_PROP += libpn553_fw.so
NQ_VENDOR_NFC_PROP += libpn557_fw.so
NQ_VENDOR_NFC_PROP += libsn100u_fw.so

ifeq ($(strip $(TARGET_USES_NQ_NFC)),true)
PRODUCT_PACKAGES += $(NQ_VENDOR_NFC_PROP)
endif
