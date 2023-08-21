# Add vendor entries from base.mk
MM_WFD_BASE_VENDOR := libwfdaac_vendor
PRODUCT_PACKAGES += $(MM_WFD_BASE_VENDOR)

# Add vendor entries from device-vendor.mk
MM_MUX_VENDOR := libFileMux_proprietary
MM_MUX_VENDOR += libOmxMux_proprietary
PRODUCT_PACKAGES += $(MM_MUX_VENDOR)

MM_RTP_VENDOR := libmmrtpencoder_proprietary
MM_RTP_VENDOR += libmmrtpdecoder_proprietary
PRODUCT_PACKAGES += $(MM_RTP_VENDOR)

WFD_VENDOR := libwfdsourcesm_proprietary
WFD_VENDOR += libwfdmmservice_proprietary
WFD_VENDOR += libwfdcodecv4l2_proprietary
WFD_VENDOR += libwfdmmsrc_proprietary
WFD_VENDOR += wifidisplayhalservice
WFD_VENDOR += vendor.qti.hardware.wifidisplaysessionl@1.0-halimpl
WFD_VENDOR += libwfdsessionmodule
WFD_VENDOR += libwfdsourcesession_proprietary
WFD_VENDOR += libwfdmodulehdcpsession
WFD_VENDOR += libwfdhdcpcp
WFD_VENDOR += libwfdhdcpservice_proprietary
WFD_VENDOR += wfdvndservice
WFD_VENDOR += wfdvndservice.rc
WFD_VENDOR += wfdhdcphalservice
WFD_VENDOR += android.hardware.drm@1.1-service.wfdhdcp.rc
WFD_VENDOR += libwfdconfigutils_proprietary
WFD_VENDOR += wfdconfig.xml
WFD_VENDOR += libOmxVideoDSMode
WFD_VENDOR += libwfdsm_proprietary
WFD_VENDOR += libwfdmminterface_proprietary
WFD_VENDOR += libwfdrtsp_proprietary
WFD_VENDOR += libwfdcommonutils_proprietary
WFD_VENDOR += libwfduibcsink_proprietary
WFD_VENDOR += libwfduibcsinkinterface_proprietary
WFD_VENDOR += libwfduibcinterface_proprietary
WFD_VENDOR += libwfduibcsrcinterface_proprietary
WFD_VENDOR += libwfduibcsrc_proprietary
WFD_VENDOR += com.qualcomm.qti.wifidisplayhal@1.0-halimpl
WFD_VENDOR += com.qualcomm.qti.wifidisplayhal@1.0
WFD_VENDOR += com.qualcomm.qti.wifidisplayhal@1.0_vendor
WFD_VENDOR += com.qualcomm.qti.wifidisplayhal@1.0-impl
WFD_VENDOR += libwfdhaldsmanager
WFD_VENDOR += libwfdmodulehdcpsession
WFD_VENDOR += com.qualcomm.qti.wifidisplayhal@1.0-service.rc
WFD_VENDOR += vendor.qti.hardware.wifidisplaysession@1.0
WFD_VENDOR += vendor.qti.hardware.wifidisplaysession@1.0_vendor
WFD_VENDOR += vendor.qti.hardware.wifidisplaysession@1.0-impl
WFD_VENDOR += ArmHDCP_QTI_Android.cfg
WFD_VENDOR += com.qualcomm.qti.wifidisplayhal@1.0-service.disable.rc
WFD_VENDOR += wfdvndservice.disable.rc
WFD_VENDOR += android.hardware.drm@1.1-service.wfdhdcp.disable.rc
WFD_VENDOR += libmiracast
WFD_VENDOR += vendor.qti.hardware.sigma_miracast@1.0_vendor
PRODUCT_PACKAGES += $(WFD_VENDOR)

