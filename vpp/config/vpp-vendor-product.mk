# APKs
PRODUCT_PACKAGES += libmmca

ifneq ($(TARGET_BOARD_AUTO),true)
PRODUCT_PACKAGES += SSMEditor
endif

# Headers
PRODUCT_PACKAGES += libvppdspheaders
PRODUCT_PACKAGES += libvpphcpheaders
PRODUCT_PACKAGES += libvppheaders
PRODUCT_PACKAGES += libvpphvxheaders
PRODUCT_PACKAGES += libvppimmotionheaders

# Native
PRODUCT_PACKAGES += libOmxVpp
PRODUCT_PACKAGES += libvppclient
PRODUCT_PACKAGES += libvpplibrary
PRODUCT_PACKAGES += vppservice

# HIDL
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.1
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.1.vendor
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.2
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.2.vendor
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.3
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.3.vendor

# Configurations
PRODUCT_PACKAGES += vpp.configstore.xml

# HY11_HY22_diff
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.1_vendor
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.2_vendor
PRODUCT_PACKAGES += vendor.qti.hardware.vpp@1.3_vendor

# Debug
PRODUCT_PACKAGES_DEBUG += libvpptestutils
PRODUCT_PACKAGES_DEBUG += vppipcunittest
PRODUCT_PACKAGES_DEBUG += vpplibraryfunctionaltest
PRODUCT_PACKAGES_DEBUG += vpplibraryunittest
PRODUCT_PACKAGES_DEBUG += vppserviceunittest
