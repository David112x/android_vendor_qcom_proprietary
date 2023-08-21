# Camera configuration file. Shared by passthrough/binderized camera HAL
PRODUCT_PACKAGES += camera.device@3.2-impl
PRODUCT_PACKAGES += camera.device@1.0-impl
PRODUCT_PACKAGES += android.hardware.camera.provider@2.4-impl
# Enable binderized camera HAL
PRODUCT_PACKAGES += android.hardware.camera.provider@2.4-service_64

#MM_CAMERA

#Camx Hardware Interface - CDK
$(call inherit-product-if-exists, $(QC_PROP_ROOT)/chi-cdk/configs/product.mk)
#copied from product.mk

#Deepportrait
MM_CAMERA += libjni_deepportrait
MM_CAMERA += libdeepportrait_system
MM_CAMERA += libSNPE_system
MM_CAMERA += libbcvsnpe_system
MM_CAMERA += libsnpe_adsp_system
MM_CAMERA += libsnpe_cdsp_system
MM_CAMERA += libsymphony-cpu_system
MM_CAMERA += libsymphonypower_system
MM_CAMERA += libc++_shared_system

#start HY11 temp fix
MM_CAMERA += com.qtistatic.stats.aec.so
MM_CAMERA += com.qtistatic.stats.af.so
MM_CAMERA += com.qtistatic.stats.awb.so
MM_CAMERA += com.qti.stats.aec.so
MM_CAMERA += com.qti.stats.af.so
MM_CAMERA += com.qti.stats.afd.so
MM_CAMERA += com.qti.stats.awb.so
MM_CAMERA += com.qti.stats.pdlib.so
MM_CAMERA += com.qti.stats.asd.so
MM_CAMERA += libcom.qti.stats.asd.so
#end HY11 temp fix

MM_CAMERA += imx318tuned.bin
MM_CAMERA += imx258tuned.bin
MM_CAMERA += com.qti.tuned.default_full_iq.bin
MM_CAMERA += libcamerarpcmem
PRODUCT_PACKAGES += $(MM_CAMERA)
