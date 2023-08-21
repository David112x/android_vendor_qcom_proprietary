/*===========================================================================
   Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
   All Rights Reserved.
   Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/

#ifndef HIDL_GENERATED_VENDOR_QTI_HARDWARE_RADIO_IMS_V1_6_IIMSRADIOINDICATION_H
#define HIDL_GENERATED_VENDOR_QTI_HARDWARE_RADIO_IMS_V1_6_IIMSRADIOINDICATION_H

#include <vendor/qti/hardware/radio/ims/1.0/types.h>
#include <vendor/qti/hardware/radio/ims/1.5/IImsRadioIndication.h>
#include <vendor/qti/hardware/radio/ims/1.5/types.h>
#include <vendor/qti/hardware/radio/ims/1.6/types.h>

#include <android/hidl/manager/1.0/IServiceNotification.h>

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace vendor {
namespace qti {
namespace hardware {
namespace radio {
namespace ims {
namespace V1_6 {

struct IImsRadioIndication : public ::vendor::qti::hardware::radio::ims::V1_5::IImsRadioIndication {
    /**
     * Type tag for use in template logic that indicates this is a 'pure' class.
     */
    typedef ::android::hardware::details::i_tag _hidl_tag;

    /**
     * Fully qualified interface name: "vendor.qti.hardware.radio.ims@1.6::IImsRadioIndication"
     */
    static const char* descriptor;

    /**
     * Returns whether this object's implementation is outside of the current process.
     */
    virtual bool isRemote() const override { return false; }

    /**
     * ImsQmiIF.UNSOL_RESPONSE_CALL_STATE_CHANGED
     *
     * @param callList List of calls and their details.
     *
     */
    virtual ::android::hardware::Return<void> onCallStateChanged(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_0::CallInfo>& callList) = 0;

    /**
     * ImsQmiIF.UNSOL_CALL_RING
     *
     */
    virtual ::android::hardware::Return<void> onRing() = 0;

    /**
     * ImsQmiIF.UNSOL_RINGBACK_TONE
     *
     * @param tone Start or stop tone.
     *
     */
    virtual ::android::hardware::Return<void> onRingbackTone(::vendor::qti::hardware::radio::ims::V1_0::ToneOperation tone) = 0;

    /**
     * ImsQmiIF.UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED
     *
     * @param registration IMS registration state.
     *
     */
    virtual ::android::hardware::Return<void> onRegistrationChanged(const ::vendor::qti::hardware::radio::ims::V1_0::RegistrationInfo& registration) = 0;

    /**
     * ImsQmiIF.UNSOL_RESPONSE_HANDOVER
     *
     * @param handover Handover details containing type, source, target
     *         and extra information about handover.
     *
     */
    virtual ::android::hardware::Return<void> onHandover(const ::vendor::qti::hardware::radio::ims::V1_0::HandoverInfo& handover) = 0;

    /**
     * Response to ImsQmiIF.UNSOL_SRV_STATUS_UPDATE
     *
     * @param srvStatusList List of service statuses.
     *
     */
    virtual ::android::hardware::Return<void> onServiceStatusChanged(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_0::ServiceStatusInfo>& srvStatusList) = 0;

    /**
     * ImsQmiIF.UNSOL_RADIO_STATE_CHANGED
     *
     * @param radioState Radio state.
     *
     */
    virtual ::android::hardware::Return<void> onRadioStateChanged(::vendor::qti::hardware::radio::ims::V1_0::RadioState radioState) = 0;

    /**
     * ImsQmiIF.UNSOL_ENTER_EMERGENCY_CALLBACK_MODE
     *
     */
    virtual ::android::hardware::Return<void> onEnterEmergencyCallBackMode() = 0;

    /**
     * ImsQmiIF.UNSOL_EXIT_EMERGENCY_CALLBACK_MODE
     *
     */
    virtual ::android::hardware::Return<void> onExitEmergencyCallBackMode() = 0;

    /**
     * ImsQmiIF.UNSOL_TTY_NOTIFICATION
     *
     * @param ttyNotify TTY info having TTY mode and additional info as bytes
     *
     */
    virtual ::android::hardware::Return<void> onTtyNotification(const ::vendor::qti::hardware::radio::ims::V1_0::TtyInfo& ttyInfo) = 0;

    /**
     * ImsQmiIF.UNSOL_REFRESH_CONF_INFO
     *
     * @param conferenceInfo Conference info having conference uri and call state
     *
     */
    virtual ::android::hardware::Return<void> onRefreshConferenceInfo(const ::vendor::qti::hardware::radio::ims::V1_0::ConferenceInfo& conferenceInfo) = 0;

    /**
     * ImsQmiIF.UNSOL_REFRESH_VICE_INFO
     *
     * @param viceInfo having VICE info URI
     *
     */
    virtual ::android::hardware::Return<void> onRefreshViceInfo(const ::vendor::qti::hardware::radio::ims::V1_0::ViceInfo& viceInfo) = 0;

    /**
     * ImsQmiIF.UNSOL_MODIFY_CALL
     *
     * @param CallModifyInfo, call modify information
     */
    virtual ::android::hardware::Return<void> onModifyCall(const ::vendor::qti::hardware::radio::ims::V1_0::CallModifyInfo& callModifyInfo) = 0;

    /**
     * ImsQmiIF.UNSOL_SUPP_SVC_NOTIFICATION
     *
     * @param suppServiceNotification details of the supplementary service notification.
     *
     */
    virtual ::android::hardware::Return<void> onSuppServiceNotification(const ::vendor::qti::hardware::radio::ims::V1_0::SuppServiceNotification& suppServiceNotification) = 0;

    /**
     * ImsQmiIF.UNSOL_MWI
     *
     * @param messageWaitingIndication details of messages waiting
     *
     */
    virtual ::android::hardware::Return<void> onMessageWaiting(const ::vendor::qti::hardware::radio::ims::V1_0::MessageWaitingIndication& messageWaitingIndication) = 0;

    /**
     * ImsQmiIF.UNSOL_REQUEST_GEOLOCATION
     *
     * @param Location coordinates for reverse-geocoding
     *
     */
    virtual ::android::hardware::Return<void> onGeolocationInfoRequested(double lat, double lon) = 0;

    /**
     * ImsQmiIF.UNSOL_IMS_SUB_CONFIG_CHANGED
     *
     * @param config IMS stack configuration on the modem.
     */
    virtual ::android::hardware::Return<void> onImsSubConfigChanged(const ::vendor::qti::hardware::radio::ims::V1_0::ImsSubConfigInfo& config) = 0;

    /**
     * ImsQmiIF.UNSOL_PARTICIPANT_STATUS_INFO
     *
     * @param ParticipantStatusInfo Participant status information.
     */
    virtual ::android::hardware::Return<void> onParticipantStatusInfo(const ::vendor::qti::hardware::radio::ims::V1_0::ParticipantStatusInfo& participantStatusInfo) = 0;

    /**
     * ImsQmiIF.UNSOL_RESPONSE_REGISTRATION_BLOCK_STATUS
     *
     * @param BlockStatusOnWwan Details of the registration block status on Wwan technology.
     * @param BlockStatusOnWlan Details of the registration block status on Wlan technology.
     *
     */
    virtual ::android::hardware::Return<void> onRegistrationBlockStatus(bool hasBlockStatusOnWwan, const ::vendor::qti::hardware::radio::ims::V1_0::BlockStatus& blockStatusOnWwan, bool hasBlockStatusOnWlan, const ::vendor::qti::hardware::radio::ims::V1_0::BlockStatus& blockStatusOnWlan) = 0;

    /**
     * ImsQmiIF.UNSOL_RESPONSE_RTT_MSG_RECEIVED
     *
     * @param message - RTT text message
     *
     */
    virtual ::android::hardware::Return<void> onRttMessageReceived(const ::android::hardware::hidl_string& message) = 0;

    /**
     * ImsQmiIF.UNSOL_VOWIFI_CALL_QUALITY
     *
     * @param VoWiFiCallQualityInfo VoWiFi call quality information
     */
    virtual ::android::hardware::Return<void> onVoWiFiCallQuality(::vendor::qti::hardware::radio::ims::V1_0::VoWiFiCallQualityInfo voWiFiCallQualityInfo) = 0;

    /**
     * ImsQmiIF.UNSOL_ON_SS
     * Indicates when Supplementary service(SS) response is received when DIAL/USSD/SS is changed
     * to SS by call control.
     *
     * @param StkCcUnsolSsResult Details of SS request and response information.
     */
    virtual ::android::hardware::Return<void> onSupplementaryServiceIndication(const ::vendor::qti::hardware::radio::ims::V1_0::StkCcUnsolSsResult& ss) = 0;

    /**
     * ImsQmiIF.UNSOL_RESPONSE_CALL_STATE_CHANGED
     *
     * @param callList List of calls and their details.
     *
     */
    virtual ::android::hardware::Return<void> onCallStateChanged_1_1(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_1::CallInfo>& callList) = 0;

    /**
     * MessageId.UNSOL_RESPONSE_CALL_STATE_CHANGED
     *
     * @param callList List of calls and their details.
     *
     */
    virtual ::android::hardware::Return<void> onCallStateChanged_1_2(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_2::CallInfo>& callList) = 0;

    /**
     * MessageId.UNSOL_IMS_SMS_STATUS_REPORT
     * Indicates the status report of the sent message.
     *
     * @param smsStatusReport ImsSmsSendStatusReport as defined in types.hal
     *
     */
    virtual ::android::hardware::Return<void> onImsSmsStatusReport(const ::vendor::qti::hardware::radio::ims::V1_2::ImsSmsSendStatusReport& smsStatusReport) = 0;

    /**
     * MessageId.UNSOL_IMS_SMS_INCOMING_SMS
     * Indicates the incoming sms over ims.
     *
     * @param imsSms ImsSmsMessage as defined in types.hal
     *
     */
    virtual ::android::hardware::Return<void> onIncomingImsSms(const ::vendor::qti::hardware::radio::ims::V1_2::IncomingImsSms& imsSms) = 0;

    /**
     * MessageId.UNSOL_VOPS_CHANGED
     *
     * @param isVopsEnabled indicates if Vops is enabled
     */
    virtual ::android::hardware::Return<void> onVopsChanged(bool isVopsEnabled) = 0;

    /**
     * MessageId.UNSOL_RESPONSE_CALL_STATE_CHANGED
     *
     * @param callList List of calls and their details.
     *
     */
    virtual ::android::hardware::Return<void> onCallStateChanged_1_3(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_3::CallInfo>& callList) = 0;

    /**
     * MessageId.UNSOL_AUTO_CALL_REJECTION_IND
     * Indicates the auto rejected incoming call and reason
     *
     * @param autoCallRejectionInfo
     */
    virtual ::android::hardware::Return<void> onIncomingCallAutoRejected(const ::vendor::qti::hardware::radio::ims::V1_3::AutoCallRejectionInfo& autoCallRejectionInfo) = 0;

    /**
     * MessageId.UNSOL_VOICE_INFO
     * Sends updates for the RTT voice info which indicates whether there is speech or silence
     * from remote user
     *
     * @param voiceInfo VoiceInfo
     *
     */
    virtual ::android::hardware::Return<void> onVoiceInfoChanged(::vendor::qti::hardware::radio::ims::V1_3::VoiceInfo voiceInfo) = 0;

    /**
     * MessageId.UNSOL_MULTI_IDENTITY_REGISTRATION_STATUS_CHANGE
     *
     * @param info List of all the lines and their registation status
     *
     */
    virtual ::android::hardware::Return<void> onMultiIdentityRegistrationStatusChange(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_4::MultiIdentityLineInfoHal>& info) = 0;

    /**
     * MessageId.ims_MsgId_UNSOL_MULTI_IDENTITY_INFO_PENDING
     *
     * Indication to ATEL that modem needs information about the
     * MultiIdentity Lines.
     *
     */
    virtual ::android::hardware::Return<void> onMultiIdentityInfoPending() = 0;

    /**
     * MessageId.UNSOL_RESPONSE_CALL_STATE_CHANGED
     *
     * @param callList List of calls and their details.
     *
     */
    virtual ::android::hardware::Return<void> onCallStateChanged_1_4(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_4::CallInfo>& callList) = 0;

    /**
     * MessageId.UNSOL_RESPONSE_CALL_STATE_CHANGED
     *
     * @param callList List of calls and their details.
     *
     */
    virtual ::android::hardware::Return<void> onCallStateChanged_1_5(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_5::CallInfo>& callList) = 0;

    /**
     * MessageId.UNSOL_AUTO_CALL_REJECTION_IND
     * Indicates the auto rejected incoming call and reason
     *
     * @param autoCallRejectionInfo
     */
    virtual ::android::hardware::Return<void> onIncomingCallAutoRejected_1_5(const ::vendor::qti::hardware::radio::ims::V1_5::AutoCallRejectionInfo& autoCallRejectionInfo) = 0;

    /**
     * MessageId.UNSOL_MODEM_SUPPORTS_WFC_ROAMING_MODE
     * Indicates if modem supports WFC roaming mode configuration
     *
     * @param wfcRoamingConfigurationSupport
     */
    virtual ::android::hardware::Return<void> onModemSupportsWfcRoamingModeConfiguration(bool wfcRoamingConfigurationSupport) = 0;

    /**
     * MessageId.UNSOL_USSD_FAILED
     * Indicates if ussd message failed over IMS
     *
     * @param type
     * @param errorDetails
     */
    virtual ::android::hardware::Return<void> onUssdMessageFailed(::vendor::qti::hardware::radio::ims::V1_5::UssdModeType type, const ::vendor::qti::hardware::radio::ims::V1_0::SipErrorInfo& errorDetails) = 0;

    /**
     * MessageId.UNSOL_RESPONSE_CALL_STATE_CHANGED
     *
     * @param callList List of calls and their details.
     *
     */
    virtual ::android::hardware::Return<void> onCallStateChanged_1_6(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_6::CallInfo>& callList) = 0;

    /**
     * MessageId.UNSOL_SRV_STATUS_UPDATE
     *
     * @param srvStatusList List of service statuses.
     *
     */
    virtual ::android::hardware::Return<void> onServiceStatusChanged_1_6(const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::ims::V1_6::ServiceStatusInfo>& srvStatusList) = 0;

    /**
     * MessageId.UNSOL_RESPONSE_IMS_NETWORK_STATE_CHANGED
     *
     * @param RegistrationInfo List of service statuses.
     *
     */
    virtual ::android::hardware::Return<void> onRegistrationChanged_1_6(const ::vendor::qti::hardware::radio::ims::V1_6::RegistrationInfo& regInfo) = 0;

    /**
     * MessageId.UNSOL_RESPONSE_HANDOVER
     *
     * @param handover Handover details containing type, source, target
     *         and extra information about handover.
     *
     */
    virtual ::android::hardware::Return<void> onHandover_1_6(const ::vendor::qti::hardware::radio::ims::V1_6::HandoverInfo& handover) = 0;

    /**
     * MessageId.UNSOL_USSD_RECEIVED
     *
     * @param type, indicates ussd mode type.
     * @param msg, message string in UTF-8, if applicable.
     * @param errorDetails, from the network.
     *
     */
    virtual ::android::hardware::Return<void> onUssdReceived(::vendor::qti::hardware::radio::ims::V1_5::UssdModeType type, const ::android::hardware::hidl_string& msg, const ::vendor::qti::hardware::radio::ims::V1_0::SipErrorInfo& errorDetails) = 0;

    /**
     * MessageId.UNSOL_CALL_COMPOSER_INFO_AVAILABLE
     *
     * @param callComposerInfo Collection of information related to pre-call. It
     *         only is populated and sent when pre-alerting state is received
     *
     */
    virtual ::android::hardware::Return<void> onCallComposerInfoAvailable(const ::vendor::qti::hardware::radio::ims::V1_6::CallComposerInfo& info) = 0;

    /**
     * MessageId.UNSOL_AUTO_CALL_COMPOSER_CALL_REJECTION_IND
     * Indicates the auto rejected incoming call and reason with call composer elements
     *
     * @param autoCallRejectionInfo
     * @param callComposerInfo
     */
    virtual ::android::hardware::Return<void> onIncomingCallComposerCallAutoRejected(const ::vendor::qti::hardware::radio::ims::V1_6::AutoCallRejectionInfo& autoCallRejectionInfo, const ::vendor::qti::hardware::radio::ims::V1_6::CallComposerInfo& callComposerInfo) = 0;

    /**
     * MessageId.UNSOL_AUTO_CALL_REJECTION_IND
     * Indicates the auto rejected incoming call and reason
     *
     * @param autoCallRejectionInfo
     */
    virtual ::android::hardware::Return<void> onIncomingCallAutoRejected_1_6(const ::vendor::qti::hardware::radio::ims::V1_6::AutoCallRejectionInfo& autoCallRejectionInfo) = 0;

    /**
     * MessageId.UNSOL_RETRIEVE_GEO_LOCATION_DATA_STATUS
     *
     * This indication will be received when modem needs location details to register over WiFi
     * and modem not able to get the location details due to GPS errors.
     *
     * @param GeoLocationDataStatus list of errors received to retrieve the geo location details.
     *
     */
    virtual ::android::hardware::Return<void> onRetrievingGeoLocationDataStatus(::vendor::qti::hardware::radio::ims::V1_6::GeoLocationDataStatus geoLocationDataStatus) = 0;

    /**
     * Return callback for interfaceChain
     */
    using interfaceChain_cb = std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& descriptors)>;
    /*
     * Provides run-time type information for this object.
     * For example, for the following interface definition:
     *     package android.hardware.foo@1.0;
     *     interface IParent {};
     *     interface IChild extends IParent {};
     * Calling interfaceChain on an IChild object must yield the following:
     *     ["android.hardware.foo@1.0::IChild",
     *      "android.hardware.foo@1.0::IParent"
     *      "android.hidl.base@1.0::IBase"]
     *
     * @return descriptors a vector of descriptors of the run-time type of the
     *         object.
     */
    virtual ::android::hardware::Return<void> interfaceChain(interfaceChain_cb _hidl_cb) override;

    /*
     * Emit diagnostic information to the given file.
     *
     * Optionally overriden.
     *
     * @param fd      File descriptor to dump data to.
     *                Must only be used for the duration of this call.
     * @param options Arguments for debugging.
     *                Must support empty for default debug information.
     */
    virtual ::android::hardware::Return<void> debug(const ::android::hardware::hidl_handle& fd, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options) override;

    /**
     * Return callback for interfaceDescriptor
     */
    using interfaceDescriptor_cb = std::function<void(const ::android::hardware::hidl_string& descriptor)>;
    /*
     * Provides run-time type information for this object.
     * For example, for the following interface definition:
     *     package android.hardware.foo@1.0;
     *     interface IParent {};
     *     interface IChild extends IParent {};
     * Calling interfaceDescriptor on an IChild object must yield
     *     "android.hardware.foo@1.0::IChild"
     *
     * @return descriptor a descriptor of the run-time type of the
     *         object (the first element of the vector returned by
     *         interfaceChain())
     */
    virtual ::android::hardware::Return<void> interfaceDescriptor(interfaceDescriptor_cb _hidl_cb) override;

    /**
     * Return callback for getHashChain
     */
    using getHashChain_cb = std::function<void(const ::android::hardware::hidl_vec<::android::hardware::hidl_array<uint8_t, 32>>& hashchain)>;
    /*
     * Returns hashes of the source HAL files that define the interfaces of the
     * runtime type information on the object.
     * For example, for the following interface definition:
     *     package android.hardware.foo@1.0;
     *     interface IParent {};
     *     interface IChild extends IParent {};
     * Calling interfaceChain on an IChild object must yield the following:
     *     [(hash of IChild.hal),
     *      (hash of IParent.hal)
     *      (hash of IBase.hal)].
     *
     * SHA-256 is used as the hashing algorithm. Each hash has 32 bytes
     * according to SHA-256 standard.
     *
     * @return hashchain a vector of SHA-1 digests
     */
    virtual ::android::hardware::Return<void> getHashChain(getHashChain_cb _hidl_cb) override;

    /*
     * This method trigger the interface to enable/disable instrumentation based
     * on system property hal.instrumentation.enable.
     */
    virtual ::android::hardware::Return<void> setHALInstrumentation() override;

    /*
     * Registers a death recipient, to be called when the process hosting this
     * interface dies.
     *
     * @param recipient a hidl_death_recipient callback object
     * @param cookie a cookie that must be returned with the callback
     * @return success whether the death recipient was registered successfully.
     */
    virtual ::android::hardware::Return<bool> linkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient, uint64_t cookie) override;

    /*
     * Provides way to determine if interface is running without requesting
     * any functionality.
     */
    virtual ::android::hardware::Return<void> ping() override;

    /**
     * Return callback for getDebugInfo
     */
    using getDebugInfo_cb = std::function<void(const ::android::hidl::base::V1_0::DebugInfo& info)>;
    /*
     * Get debug information on references on this interface.
     * @return info debugging information. See comments of DebugInfo.
     */
    virtual ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb) override;

    /*
     * This method notifies the interface that one or more system properties
     * have changed. The default implementation calls
     * (C++)  report_sysprop_change() in libcutils or
     * (Java) android.os.SystemProperties.reportSyspropChanged,
     * which in turn calls a set of registered callbacks (eg to update trace
     * tags).
     */
    virtual ::android::hardware::Return<void> notifySyspropsChanged() override;

    /*
     * Unregisters the registered death recipient. If this service was registered
     * multiple times with the same exact death recipient, this unlinks the most
     * recently registered one.
     *
     * @param recipient a previously registered hidl_death_recipient callback
     * @return success whether the death recipient was unregistered successfully.
     */
    virtual ::android::hardware::Return<bool> unlinkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient) override;

    // cast static functions
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_5::IImsRadioIndication>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_4::IImsRadioIndication>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_3::IImsRadioIndication>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_2::IImsRadioIndication>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_1::IImsRadioIndication>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_0::IImsRadioIndication>& parent, bool emitError = false);
    /**
     * This performs a checked cast based on what the underlying implementation actually is.
     */
    static ::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>> castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError = false);

    // helper methods for interactions with the hwservicemanager
    /**
     * This gets the service of this type with the specified instance name. If the
     * service is currently not available or not in the VINTF manifest on a Trebilized
     * device, this will return nullptr. This is useful when you don't want to block
     * during device boot. If getStub is true, this will try to return an unwrapped
     * passthrough implementation in the same process. This is useful when getting an
     * implementation from the same partition/compilation group.
     *
     * In general, prefer getService(std::string,bool)
     */
    static ::android::sp<IImsRadioIndication> tryGetService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IImsRadioIndication> tryGetService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return tryGetService(str, getStub); }
    /**
     * Deprecated. See tryGetService(std::string, bool)
     */
    static ::android::sp<IImsRadioIndication> tryGetService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return tryGetService(str, getStub); }
    /**
     * Calls tryGetService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IImsRadioIndication> tryGetService(bool getStub) { return tryGetService("default", getStub); }
    /**
     * This gets the service of this type with the specified instance name. If the
     * service is not in the VINTF manifest on a Trebilized device, this will return
     * nullptr. If the service is not available, this will wait for the service to
     * become available. If the service is a lazy service, this will start the service
     * and return when it becomes available. If getStub is true, this will try to
     * return an unwrapped passthrough implementation in the same process. This is
     * useful when getting an implementation from the same partition/compilation group.
     */
    static ::android::sp<IImsRadioIndication> getService(const std::string &serviceName="default", bool getStub=false);
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IImsRadioIndication> getService(const char serviceName[], bool getStub=false)  { std::string str(serviceName ? serviceName : "");      return getService(str, getStub); }
    /**
     * Deprecated. See getService(std::string, bool)
     */
    static ::android::sp<IImsRadioIndication> getService(const ::android::hardware::hidl_string& serviceName, bool getStub=false)  { std::string str(serviceName.c_str());      return getService(str, getStub); }
    /**
     * Calls getService("default", bool). This is the recommended instance name for singleton services.
     */
    static ::android::sp<IImsRadioIndication> getService(bool getStub) { return getService("default", getStub); }
    /**
     * Registers a service with the service manager. For Trebilized devices, the service
     * must also be in the VINTF manifest.
     */
    __attribute__ ((warn_unused_result))::android::status_t registerAsService(const std::string &serviceName="default");
    /**
     * Registers for notifications for when a service is registered.
     */
    static bool registerForNotifications(
            const std::string &serviceName,
            const ::android::sp<::android::hidl::manager::V1_0::IServiceNotification> &notification);
};

//
// type declarations for package
//

static inline std::string toString(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>& o);

//
// type header definitions for package
//

static inline std::string toString(const ::android::sp<::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication>& o) {
    std::string os = "[class or subclass of ";
    os += ::vendor::qti::hardware::radio::ims::V1_6::IImsRadioIndication::descriptor;
    os += "]";
    os += o->isRemote() ? "@remote" : "@local";
    return os;
}


}  // namespace V1_6
}  // namespace ims
}  // namespace radio
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

//
// global type declarations for package
//


#endif  // HIDL_GENERATED_VENDOR_QTI_HARDWARE_RADIO_IMS_V1_6_IIMSRADIOINDICATION_H
