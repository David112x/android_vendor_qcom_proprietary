# Packages associated with Global Platform support
ifneq ($(TARGET_PRODUCT),bengal_32go)
PRODUCT_PACKAGES += libGPTEE_vendor
PRODUCT_PACKAGES += libGPMTEEC_vendor
PRODUCT_PACKAGES += libGPQTEEC_vendor
PRODUCT_PACKAGES += libQTEEConnector_vendor

PRODUCT_PACKAGES += vendor.qti.hardware.qteeconnector@1.0
PRODUCT_PACKAGES += vendor.qti.hardware.qteeconnector@1.0-impl
PRODUCT_PACKAGES += vendor.qti.hardware.qteeconnector@1.0-service
PRODUCT_PACKAGES += vendor.qti.hardware.qteeconnector@1.0-service.rc
PRODUCT_PACKAGES += vendor.qti.hardware.qteeconnector@1.0_vendor
endif
