#DISPLAY
PRODUCT_PACKAGES += libsdmextension \
                    libscalar \
                    libqseed3 \
                    libsdm-disp-apis \
                    libsdm-color \
                    libsdm-diag \
                    libhdr_tm \
                    libhdrdynamic \
                    libhdrdynamicootf \
                    libgame_enhance \
                    libdisplayskuutils \
                    libgtestext \
                    libxmlext \
                    libsdedrm \
                    libdisplayqos \
                    libtinyxml2_1 \
                    libsdm-disp-vndapis \
                    libsdm-colormgr-algo \
                    libdpps \
                    libdisp-aba \
                    libmm-abl \
                    libmm-abl-oem \
                    libscale \
                    libmm-hdcpmgr \
                    libmm-qdcm \
                    libmm-disp-apis \
                    libmm-als \
                    QdcmFF \
                    qdcmss \
                    qdcmss.rc \
                    CABLService \
                    SVIService \
                    libsnapdragoncolor

ifneq ($(TARGET_BOARD_PLATFORM),bengal)
PRODUCT_PACKAGES += feature_enabler_client \
                    feature_enabler_client.rc
endif

ifneq ($(TARGET_HAS_LOW_RAM),true)
PRODUCT_PACKAGES += vendor.display.color@1.0.vendor \
                    vendor.display.color@1.1.vendor \
                    vendor.display.color@1.2.vendor \
                    vendor.display.color@1.3.vendor \
                    vendor.display.color@1.4.vendor \
                    vendor.display.color@1.5.vendor \
                    vendor.display.color@1.6.vendor \
                    vendor.display.postproc@1.0.vendor \
                    vendor.display.color@1.0-service \
                    vendor.display.color@1.0-service.rc \
                    ppd
endif

PRODUCT_PACKAGES_DEBUG += testmgr
