/******************************************************************************
#  Copyright (c) 2008-2009, 2012-2013, 2017, 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/*!
  @file
  qcril_qmi_voice.h

  @brief

*/


/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.


when       who     what, where, why
--------   ---     ----------------------------------------------------------
08/06/10   sk      Initial Changes

===========================================================================*/

#ifndef QCRIL_QMI_VOICE_H
#define QCRIL_QMI_VOICE_H

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "comdef.h"
#include "qmi_client.h"
#include "voice_service_v02.h"
#include "qcril_qmi_pil_monitor.h"
#include "qcril_pd_notifier.h"
#include "qcrili.h"
#include "qcril_qmi_client.h"
#include <interfaces/voice/voice.h>
#include <interfaces/voice/QcRilRequestGetCurrentCallsMessage.h>
#include <interfaces/voice/QcRilRequestGetCurrentSetupCallsMessage.h>

/*===========================================================================

                        DEFINITIONS AND TYPES

===========================================================================*/

#define QCRIL_QMI_VOICE_RIL_PI_ALLOWED 0
#define QCRIL_QMI_VOICE_RIL_PI_RESTRICTED 1
#define QCRIL_QMI_VOICE_RIL_PI_UNKNOWN 2
#define QCRIL_QMI_VOICE_RIL_PI_PAYPHONE 3

#define QCRIL_QMI_VOICE_INVALID_CONN_ID         0
#define QCRIL_QMI_VOICE_UNKNOWN_ACTIVE_CONN_ID  0xFF

#define QCRIL_QMI_VOICE_INTERCODING_BUF_LEN     1024
#define QCRIL_QMI_VOICE_DIAL_NUMBER_MAX_LEN     256

#define QCRIL_QMI_VOICE_SUBADDRESS_IA5_IDENTIFIER 0x50

#define QCRIL_QMI_VOICE_FAIL_CAUSE_STR_LEN     256

//Value defined as per ATT requirement
#define QCRIL_QMI_VOICE_DTMF_INTERVAL_VAL 150

/* CLIR Persistent System Property */
#define QCRIL_QMI_VOICE_CLIR                         "persist.vendor.radio.clir"
#define QCRIL_QMI_VOICE_CLIR_FACNOTSUP_AS_NONW       "persist.vendor.radio.facnotsup_as_nonw"
#define QCRIL_QMI_AUTO_ANSWER                        "persist.vendor.radio.autoanswer.ms"
#define QCRIL_QMI_VOICE_REPORT_SPEECH_CODEC          "persist.vendor.radio.report_codec"
#define QMI_RIL_SYS_PROP_NAME_SUBADDRESS             "persist.vendor.radio.support_subaddr"
#define QMI_RIL_SYS_PROP_NAME_SUBADDRESS_AMPERSAND   "persist.vendor.radio.subaddr_amp"
#define QCRIL_REJECT_CAUSE_21_SUPPORTED              "persist.vendor.radio.reject_cause_21"
#define QMI_RIL_SYS_PROP_NAME_REDIR_PARTY_NUM        "persist.vendor.radio.redir_party_num"
#define QCRIL_QMI_VOICE_DTMF_INTERVAL                "ro.vendor.ril.dtmf_interval"
#define QCRIL_QMI_VOICE_DTMF_PAUSE_INTERVAL          "ro.vendor.ril.dtmf_pause_interval"
#define QCRIL_QMI_CDMA_VOICE_EMER_VOICE              "persist.vendor.radio.call_type"
#define QCRIL_QMI_SUPPORT_ENCRYPTED_CALLS            "vendor.radio.support.encrypted_calls"
#define QMI_RIL_SYS_PROP_NAME_SUBADDRESS_IA5_IDENTIFIER         "persist.vendor.radio.subaddr_ia5_id"
#define QCRIL_QMI_VOICE_FAKE_AUDIO_UP                "persist.vendor.radio.fake_audio_up"
#define QCRIL_INFORCE_0X9E_TREAT_AS_TAG              "persist.vendor.radio.0x9e_not_callname"
#define QCRIL_PROCESS_SUPS_IND                       "persist.vendor.radio.process_sups_ind"
#define QCRIL_PROCESS_ORIGINAL_NUMBER_DISPLAY        "persist.vendor.radio.orig_num_disp"

#define QCRIL_QMI_VOICE_SS_TA_UNKNOWN       129 /* 0x80|CM_TON_UNKNOWN      |CM_NPI_ISDN */
#define QCRIL_QMI_VOICE_SS_TA_INTERNATIONAL 145 /* 0x80|CM_TON_INTERNATIONAL|CM_NPI_ISDN */
#define QCRIl_QMI_VOICE_SS_TA_INTER_PREFIX  '+' /* ETSI international call dial prefix */

// backport from cm.h -- start
#define SYS_MAX_PRIMARY_PDP_CONTEXTS          3
#define SYS_MAX_SEC_PDP_CONTEXT_PER_PRIMARY   2

#define SYS_MAX_PDP_CONTEXTS                                                 \
  ( SYS_MAX_PRIMARY_PDP_CONTEXTS +                                           \
    ( SYS_MAX_PRIMARY_PDP_CONTEXTS * SYS_MAX_SEC_PDP_CONTEXT_PER_PRIMARY ))

#define CM_CALL_ID_MAX              ( 6 + SYS_MAX_PDP_CONTEXTS + \
                                      CM_MAX_EPS_BEARERS_CONTEXTS )

#define SYS_MAX_EPS_BEARERS_CONTEXTS          8

#define CM_MAX_EPS_BEARERS_CONTEXTS ( SYS_MAX_EPS_BEARERS_CONTEXTS - \
                                      SYS_MAX_PRIMARY_PDP_CONTEXTS )

#define INVALID_MEDIA_ID -1

// backport from cm.h -- end

#define QCRIL_QMI_VOICE_MAX_MT_USSD_CHAR  183
#define QCRIL_QMI_VOICE_ALPHA_LENGTH_IN_NULL_CASE 1
#define QCRIL_QMI_VOICE_MAX_SUPS_FAILURE_STR_LEN  256

//Should be the same value as defined in QMI VOICE
//interface file - QMI_VOICE_IP_FORWARD_HIST_INFO_MAX_LEN_V02
#define QCRIL_QMI_VOICE_MAX_IP_HISTORY_INFO_LEN  512

#define INVALID_NEGATIVE_ONE -1

//Types of qmi info ind:
//Conference info indication - Reports information about conference participant
//Dialog info indicaiton - Reports information about calls on primary and secondary devices
#define MAX_QMI_INFO_IND_XML 2

#define QCRIL_QMI_VOICE_CFW_RESPONSE_BUF_SZ   7

typedef enum {
  SIP_TRYING=100,
  SIP_RINGING=180,
  SIP_CALL_IS_BEING_FORWARDED=181,
  SIP_QUEUED=182,
  SIP_SESSION_PROGRESS=183,
  SIP_OK=200,
  SIP_ACCEPTED=202,
  SIP_MULTIPLE_CHOICES=300,
  SIP_MOVED_PERMANENTLY=301,
  SIP_MOVED_TEMPORARILY=302,
  SIP_USE_PROXY=305,
  SIP_ALTERNATIVE_SERVICE=380,
  SIP_BAD_REQUEST=400,
  SIP_UNAUTHORIZED=401,
  SIP_PAYMENT_REQUIRED=402,
  SIP_FORBIDDEN=403,
  SIP_NOTFOUND=404,
  SIP_METHOD_NOT_ALLOWED=405,
  SIP_HEADER_NOT_ACCEPTABLE=406,
  SIP_PROXY_AUTHENTICATION_REQUIRED=407,
  SIP_REQUEST_TIMEOUT=408,
  SIP_GONE=410,
  SIP_REQUEST_ENTITY_TOO_LARGE=413,
  SIP_REQUEST_URI_TOO_LONG=414,
  SIP_UNSUPPORTED_MEDIA_TYPE=415,
  SIP_UNSUPPORTED_URI_SCHEME=416,
  SIP_BAD_EXTENSION=420,
  SIP_EXTENSION_REQUIRED=421,
  SIP_INTERVAL_TOO_BRIEF=423,
  SIP_TEMPORARILY_UNAVAILABLE=480,
  SIP_CALL_TRANSACTION_DOES_NOT_EXIST=481,
  SIP_LOOP_DETECTED=482,
  SIP_TOO_MANY_HOPS=483,
  SIP_ADDRESS_INCOMPLETE=484,
  SIP_AMBIGUOUS=485,
  SIP_BUSY_HERE=486,
  SIP_REQUEST_TERMINATED=487,
  SIP_NOT_ACCEPTABLE_HERE=488,
  SIP_BAD_EVENT=489,
  SIP_REQUEST_PENDING=491,
  SIP_UNDECIPHERABLE=493,
  SIP_SERVER_INTERNAL_ERROR=500,
  SIP_NOT_IMPLEMENTED=501,
  SIP_BAD_GATEWAY=502,
  SIP_SERVICE_UNAVAILABLE=503,
  SIP_SERVER_TIMEOUT=504,
  SIP_VERSION_NOT_SUPPORTED=505,
  SIP_MESSAGE_TOO_LARGE=513,
  SIP_BUSY_EVERYWHERE=600,
  SIP_DECLINE=603,
  SIP_DOES_NOT_EXIST_ANYWHERE=604,
  SIP_MEDIA_NOT_ACCEPTABLE=606,
  SIP_TRY_AFTER=10000,
}sip_error_code_enum_v02;


typedef enum qcril_qmi_voice_ss_supps_notification_mo_e
{
  QCRIL_QMI_VOICE_SS_CSSI_ORIG_FWD_STAT         = -1,
  QCRIL_QMI_VOICE_SS_CSSI_UNCOND_FWD_ACTIVE     = 0,
  QCRIL_QMI_VOICE_SS_CSSI_COND_FWD_ACTIVE       = 1,
  QCRIL_QMI_VOICE_SS_CSSI_CALL_FORWARDED        = 2,
  QCRIL_QMI_VOICE_SS_CSSI_CALL_WAITING          = 3,
  QCRIL_QMI_VOICE_SS_CSSI_CUG_CALL              = 4,
  QCRIL_QMI_VOICE_SS_CSSI_OUTGOING_CALLS_BARRED = 5,
  QCRIL_QMI_VOICE_SS_CSSI_INCOMING_CALLS_BARRED = 6,
  QCRIL_QMI_VOICE_SS_CSSI_CLIR_SUPPRESSION_REJ  = 7,
  QCRIL_QMI_VOICE_SS_CSSI_CALL_DEFLECTED        = 8,
  QCRIL_QMI_VOICE_SS_CSSI_MAX
}qcril_qmi_voice_ss_supps_notification_mo_e_type;

typedef enum qcril_qmi_voice_ss_supps_notification_mt_e
{
  QCRIL_QMI_VOICE_SS_CSSU_FORWARDED_CALL                = 0,
  QCRIL_QMI_VOICE_SS_CSSU_CUG_CALL                      = 1,
  QCRIL_QMI_VOICE_SS_CSSU_CALL_HOLD                     = 2,
  QCRIL_QMI_VOICE_SS_CSSU_CALL_RETRIEVED                = 3,
  QCRIL_QMI_VOICE_SS_CSSU_MPTY_CALL                     = 4,
  QCRIL_QMI_VOICE_SS_CSSU_CALL_HOLD_RELEASED            = 5,
  QCRIL_QMI_VOICE_SS_CSSU_FWD_CHECK_SS_RECVD            = 6,
  QCRIL_QMI_VOICE_SS_CSSU_ECT_CALL_REMOTE_PTY_ALERT     = 7,
  QCRIL_QMI_VOICE_SS_CSSU_ECT_CALL_REMOTE_PTY_CONNECTED = 8,
  QCRIL_QMI_VOICE_SS_CSSU_DEFLECTED_CALL                = 9,
  QCRIL_QMI_VOICE_SS_CSSU_ADDITIONAL_INCOM_CALL_FWD     = 10,
  QCRIL_QMI_VOICE_SS_CSSU_MAX
}qcril_qmi_voice_ss_supps_notification_mt_e_type;

typedef enum qcril_qmi_ss_notification_type_e
{
 QCRIL_QMI_VOICE_SS_MIN_NOTIFICATION = -1,
 QCRIL_QMI_VOICE_SS_MO_NOTIFICATION = 0,
 QCRIL_QMI_VOICE_SS_MT_NOTIFICATION = 1,
 QCRIL_QMI_VOICE_SS_MAX_NOTIFICATION
}qcril_qmi_voice_ss_notification_type_e_type;

/* CLIR enable/disable settings */
typedef enum qcril_qmi_voice_ss_clir_type_e{
  QCRIL_QMI_VOICE_SS_CLIR_PRESENTATION_INDICATOR = 0,
  QCRIL_QMI_VOICE_SS_CLIR_INVOCATION_OPTION  = 1,
  QCRIL_QMI_VOICE_SS_CLIR_SUPPRESSION_OPTION = 2
}qcril_qmi_voice_ss_clir_type_e_type;

/*Type of qmi info indication xml*/
typedef enum qcril_qmi_voice_info_ind_xml_type_e{
  QCRIL_QMI_VOICE_CONFERENCE_INFO_IND_XML = 0,
  QCRIL_QMI_VOICE_DIALOG_INFO_IND_XML     = 1,
  QCRIL_QMI_VOICE_MAX_INFO_IND_XML
}qcril_qmi_voice_info_ind_xml_type_e_type;

typedef struct
{
  RIL_Call *info_ptr[ CM_CALL_ID_MAX ];
  RIL_Call info[ CM_CALL_ID_MAX ];
  RIL_UUS_Info uus_info[ CM_CALL_ID_MAX ];
  uint8_t codec_valid[ CM_CALL_ID_MAX ];
  voice_speech_codec_enum_v02 codec[ CM_CALL_ID_MAX ];
  uint8_t local_call_capabilities_info_valid[ CM_CALL_ID_MAX ];
  voice_ip_call_capabilities_info_type_v02 local_call_capabilities_info[ CM_CALL_ID_MAX ];
  uint8_t peer_call_capabilities_info_valid[ CM_CALL_ID_MAX ];
  voice_ip_call_capabilities_info_type_v02 peer_call_capabilities_info[ CM_CALL_ID_MAX ];
  uint8_t rtt_mode_valid[ CM_CALL_ID_MAX ];
  voice_call_rtt_mode_status_type_v02 rtt_mode[ CM_CALL_ID_MAX ];
  uint8_t rtt_capabilities_info_type_valid [ CM_CALL_ID_MAX ];
  voice_call_rtt_capabilities_info_type_v02 rtt_capabilities_info_type[ CM_CALL_ID_MAX ];
  uint8_t child_number_valid[ CM_CALL_ID_MAX ];
  char *child_number[ CM_CALL_ID_MAX ];
  uint8_t display_text_valid[ CM_CALL_ID_MAX ];
  char *display_text[ CM_CALL_ID_MAX ];
  uint8_t additional_call_info_valid[ CM_CALL_ID_MAX ];
  char *additional_call_info[ CM_CALL_ID_MAX ];
  call_mode_enum_v02 mode[ CM_CALL_ID_MAX ];
  boolean lcf_valid[ CM_CALL_ID_MAX ];
  qcril::interfaces::CallFailCause lcf[ CM_CALL_ID_MAX ];
  call_end_reason_enum_v02 lcf_extended_codes[ CM_CALL_ID_MAX ];
  int32_t media_id[CM_CALL_ID_MAX];
  uint8_t end_reason_text_valid[ CM_CALL_ID_MAX ];
  char *end_reason_text[ CM_CALL_ID_MAX ];
  uint8_t call_modified_cause_valid[ CM_CALL_ID_MAX ];
  voice_call_modified_cause_enum_v02 call_modified_cause[ CM_CALL_ID_MAX ];
  uint8_t is_secure_call[ CM_CALL_ID_MAX ];
  uint8_t is_called_party_ringing_valid[ CM_CALL_ID_MAX ];
  uint8_t is_called_party_ringing[ CM_CALL_ID_MAX ];
  uint8_t sip_error_code_valid[ CM_CALL_ID_MAX ];
  uint16_t sip_error_code [ CM_CALL_ID_MAX ];
  uint8_t alternate_sip_uris_valid[ CM_CALL_ID_MAX ];
  char* alternate_sip_uris [ CM_CALL_ID_MAX ];
  uint8_t history_info_valid [ CM_CALL_ID_MAX ];
  char *history_info [ CM_CALL_ID_MAX ];
  uint8_t remote_party_conf_capability_valid[ CM_CALL_ID_MAX ];
  uint8_t remote_party_conf_capability[ CM_CALL_ID_MAX ];
  uint8_t verstat_info_valid[CM_CALL_ID_MAX];
  voice_MT_call_received_verstat_type_v02 verstat_info[CM_CALL_ID_MAX];
  uint8_t terminating_num_valid[CM_CALL_ID_MAX];
  char* terminating_num[CM_CALL_ID_MAX];
  uint8_t is_secondary[CM_CALL_ID_MAX];
  qcril::interfaces::CallType callType[ CM_CALL_ID_MAX ];
  qcril::interfaces::CallDomain callDomain[ CM_CALL_ID_MAX ];
  uint32_t callSubState[ CM_CALL_ID_MAX ];
  uint8_t emerg_srv_categ_valid[CM_CALL_ID_MAX];
  uint32_t emerg_srv_categ[CM_CALL_ID_MAX];
  sups_mode_enum_v02 tir_mode[CM_CALL_ID_MAX];
  bool tir_mode_valid[CM_CALL_ID_MAX];
  uint32 num_of_calls;
} qcril_qmi_voice_current_calls_type;

/* Call Independent supplimenary services modes
     like de-activation, activation, registration , erasure etc..
*/
typedef enum qcril_qmi_voice_ss_mode_e
{
  QCRIL_QMI_VOICE_MODE_DISABLE    = 0,
  QCRIL_QMI_VOICE_MODE_ENABLE     = 1,
  QCRIL_QMI_VOICE_MODE_QUERY      = 2,
  QCRIL_QMI_VOICE_MODE_REG        = 3,
  QCRIL_QMI_VOICE_MODE_ERASURE    = 4,
  QCRIL_QMI_VOICE_MODE_REG_PASSWD = 5,
  QCRIL_QMI_VOICE_MODE_MAX
} qcril_qmi_voice_mode_e_type;

/* different types of call forwarding SS */
typedef enum qcril_qmi_voice_ccfc_reason_e
{
  QCRIL_QMI_VOICE_CCFC_REASON_UNCOND    = 0,
  QCRIL_QMI_VOICE_CCFC_REASON_BUSY      = 1,
  QCRIL_QMI_VOICE_CCFC_REASON_NOREPLY   = 2,
  QCRIL_QMI_VOICE_CCFC_REASON_NOTREACH  = 3,
  QCRIL_QMI_VOICE_CCFC_REASON_ALLCALL   = 4,
  QCRIL_QMI_VOICE_CCFC_REASON_ALLCOND   = 5,
  QCRIL_QMI_VOICE_CCFC_REASON_MAX
} qcril_qmi_voice_ccfc_reason_e_type;

/* This enum represents the CLIR provisioning status at the network */
typedef enum qcril_qmi_voice_clir_status_e {
  QCRIL_QMI_VOICE_CLIR_SRV_NOT_PROVISIONED         = 0,
  QCRIL_QMI_VOICE_CLIR_SRV_PROVISIONED_PERMANENT   = 1,
  QCRIL_QMI_VOICE_CLIR_SRV_NO_NETWORK              = 2,
  QCRIL_QMI_VOICE_CLIR_SRV_PRESENTATION_RESTRICTED = 3,
  QCRIL_QMI_VOICE_CLIR_SRV_PRESENTATION_ALLOWED    = 4
} qcril_qmi_voice_clir_status_e_type;

typedef enum qcril_qmi_voice_ussd_dcs_e {

  QCRIL_QMI_VOICE_USSD_DCS_NONE = -1,
    /**< @internal */

  QCRIL_QMI_VOICE_USSD_DCS_7_BIT = 0x00,
    /**< 7 bit Data encoding scheme used for ussd */

  QCRIL_QMI_VOICE_USSD_DCS_8_BIT = 0x94,
    /**< 8 bit Data encoding scheme used for ussd */

  QCRIL_QMI_VOICE_USSD_DCS_UCS2  = 0x98,
    /**< Universal multi-octet character set encoding
    ** Clients need to check for CM_API_USSD_DCS
    ** and CM_API_USSD_UCS2_DCS
    */

  QCRIL_QMI_VOICE_USSD_DCS_UNSPECIFIED = 0x0F,
    /**< Data encoding scheme unspecified */

  QCRIL_QMI_VOICE_USSD_DCS_MAX
    /**< @internal */

} qcril_qmi_voice_ussd_dcs_e_type;

typedef enum qcril_qmi_voice_stk_cc_modification_e
{
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_NONE,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_TO_DIAL,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_TO_DIAL_VIDEO,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_TO_SS,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_TO_USSD,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_SS_TO_DIAL,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_SS_TO_DIAL_VIDEO,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_SS_TO_SS,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_SS_TO_USSD,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_USSD_TO_DIAL,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_USSD_TO_DIAL_VIDEO,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_USSD_TO_SS,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_USSD_TO_USSD,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_VIDEO_TO_DIAL,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_VIDEO_TO_DIAL_VIDEO,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_VIDEO_TO_SS,
    QCRIL_QMI_VOICE_STK_CC_MODIFICATION_DIAL_VIDEO_TO_USSD,
} qcril_qmi_voice_stk_cc_modification_e_type;

typedef struct
{
    qcril_qmi_voice_stk_cc_modification_e_type  modification;
    voice_cc_sups_result_type_v02               ss_ussd_info;
    uint8                                       call_id_info;
    uint8                                       is_alpha_relayed;
    voice_alpha_ident_type_v02                  alpha_ident;
    boolean                                     is_ims_request;
} qcril_qmi_voice_stk_cc_info_type;

typedef struct
{
    boolean               last_call_failure_cause_valid;
    qcril::interfaces::CallFailCause last_call_failure_cause;
    std::string           last_call_failure_cause_str;
    boolean               pending_req;
    uint32                pending_request_timeout_timer_id;
    qtimutex::QtiSharedMutex call_failure_cause_lock_mutex;
} qcril_qmi_voice_last_call_failure_cause;

typedef struct
{
    char emergency_number[QCRIL_QMI_VOICE_DIAL_NUMBER_MAX_LEN];
    char ims_address[QCRIL_QMI_VOICE_DIAL_NUMBER_MAX_LEN];
} qcril_qmi_voice_emer_num_ims_addr_info_type;

typedef struct
{
   uint32 total_size;
   uint32 filled_size;
   int8  last_sequence_number;
   uint8* buffer;
   boolean call_id_valid;
   uint8_t call_id;
} qcril_qmi_voice_info_ind_xml_type;

typedef struct qcril_qmi_voice_pd_info
{
   qcril_qmi_pd_handle *handle;
   qcril_qmi_pd_state state; /* Overall state */
   pthread_t pd_init_thread;
   boolean pd_init_thread_valid;
} qcril_qmi_voice_pd_info;
typedef enum qcril_qmi_audio_state
{
  QCRIL_QMI_AUDIO_STATE_UNKNOWN,
  QCRIL_QMI_AUDIO_STATE_UP,
  QCRIL_QMI_AUDIO_STATE_DOWN,
} qcril_qmi_audio_state;
typedef struct  qcril_qmi_audio_info
{
    qcril_qmi_audio_state state;
} qcril_qmi_audio_info;
typedef struct
{
  qtimutex::QtiSharedMutex voice_info_lock_mutex;
  qcril_qmi_voice_last_call_failure_cause last_call_failure_cause;
  boolean last_call_is_local_ringback;
  uint8_t last_local_ringback_call_id;
  uint8 clir;
  uint8 ussd_user_action_required;
  qcril_qmi_pil_state pil_state;
  qcril_qmi_voice_pd_info pd_info;
  qcril_qmi_audio_info audio_info;
  qcril_qmi_voice_info_ind_xml_type qmi_info_ind_xml[MAX_QMI_INFO_IND_XML];
  boolean send_vice_unsol_on_socket_connect;
  std::vector<voice_ussd_ind_msg_v02> ussd_info_buffer_for_opt;
}qcril_qmi_voice_info_type;

typedef struct
{
  int is_valid;
  int toa;
  char *number;
  int  numberPresentation;
}qcril_qmi_voice_emer_voice_feature_info_type;

typedef uint64_t qcril_qmi_voice_voip_call_info_elaboration_type;
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_NONE                              ((uint64_t) 0)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_FIRST                             (((uint64_t) 1) << 0)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_GOLDEN                         (((uint64_t) 1) << 0)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CS_DOMAIN                         (((uint64_t) 1) << 1)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_PS_DOMAIN                         (((uint64_t) 1) << 2)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_RTT_MODE_VALID                    (((uint64_t) 1) << 3)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_NUMBER_VALID              (((uint64_t) 1) << 4)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_NAME_VALID                (((uint64_t) 1) << 5)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_ALERTING_TYPE_VALID               (((uint64_t) 1) << 6)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_SIP_ERROR_CODE_VALID              (((uint64_t) 1) << 7)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_ALTERNATE_SIP_URI_VALID           (((uint64_t) 1) << 8)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_VERSTAT_INFO_VALID                (((uint64_t) 1) << 9)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CONN_PARTY_NUM_VALID              (((uint64_t) 1) << 10)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_TERMINATING_NUM_VALID             (((uint64_t) 1) << 11)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EMERG_SRV_CATEG_VALID             (((uint64_t) 1) << 12)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_REDIRECTING_PARTY_NUM_VALID       (((uint64_t) 1) << 13)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_MO_CALL_BEING_SETUP               (((uint64_t) 1) << 14)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_TIR_MODE_VALID                    (((uint64_t) 1) << 15)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_VOICE_PRIVACY_VALID               (((uint64_t) 1) << 16)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_UUS_VALID                         (((uint64_t) 1) << 17)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_RIL_CALL_STATE_VALID              (((uint64_t) 1) << 18)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_SRVCC_VALID                    (((uint64_t) 1) << 19)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_PENDING_INCOMING                  (((uint64_t) 1) << 20)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_WAITING_FOR_MATCHING_VOICE_RTE    (((uint64_t) 1) << 21)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_GOT_CONNECTED                (((uint64_t) 1) << 22)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_NEED_FOR_RING_PENDING             (((uint64_t) 1) << 23)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_NO_RING_DONE                      (((uint64_t) 1) << 24)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EME_FROM_OOS                      (((uint64_t) 1) << 25)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EXTENDED_DIALING                  (((uint64_t) 1) << 27)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EXTENDED_DIALING_ENDING           (((uint64_t) 1) << 28)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_AUDIO_ATTR_VALID                  (((uint64_t) 1) << 29)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_VIDEO_ATTR_VALID                  (((uint64_t) 1) << 30)
#define QCRIL_QMI_VOICE_VOIP_RINING_TIME_ID_VALID                           (((uint64_t) 1) << 31)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_ENDED_REPORTED               (((uint64_t) 1) << 32)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_RTT_CAP_INFO_VALID            (((uint64_t) 1) << 33)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_LAST_CALL_FAILURE_REPORTED        (((uint64_t) 1) << 34)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_END_BY_USER                  (((uint64_t) 1) << 35)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_REPORT_CACHED_RP_NUMBER           (((uint64_t) 1) << 36)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IP_CALLER_NAME_VALID              (((uint64_t) 1) << 37)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_1X_REMOTE_NUM_PENDING             (((uint64_t) 1) << 38)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EMERGENCY_CALL                    (((uint64_t) 1) << 39)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_1x_CSFB_CALL                      (((uint64_t) 1) << 40)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_DIAL_FROM_OOS                     (((uint64_t) 1) << 41)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_END_REASON_TEXT_VALID             (((uint64_t) 1) << 42)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_MOD_CAUSE_VALID              (((uint64_t) 1) << 43)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_CALLED_PARTY_RING_VALID        (((uint64_t) 1) << 44)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_HISTORY_INFO_VALID                (((uint64_t) 1) << 45)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_AUTO_DOMAIN                       (((uint64_t) 1) << 46)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_NO_QMI_ID_RECEIVED                (((uint64_t) 1) << 47)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_MODIFY_CONFIRM_PENDING       (((uint64_t) 1) << 48)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_ATTR_VALID                   (((uint64_t) 1) << 49)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CODEC_VALID                       (((uint64_t) 1) << 50)
// CONF_PATICIAPNT_CALL_END_REPORTED elab is not used now
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_CONF_CAP_VALID            (((uint64_t) 1) << 51)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_PHANTOM_CALL                      (((uint64_t) 1) << 52)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_LOCAL_CALL_CAPBILITIES_VALID      (((uint64_t) 1) << 53)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_PEER_CALL_CAPBILITIES_VALID       (((uint64_t) 1) << 54)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_CHILD_NUMBER_VALID             (((uint64_t) 1) << 55)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_DISPLAY_TEXT_VALID             (((uint64_t) 1) << 56)

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_ANSWERING_CALL                    (((uint64_t) 1) << 57)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_SWITCHING_CALL_TO_ACTIVE          (((uint64_t) 1) << 58)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_END_AFTER_SRVCC              (((uint64_t) 1) << 59)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_HANGUP_AFTER_VALID_QMI_ID         (((uint64_t) 1) << 60)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EMER_NUM_TO_IMS_ADDR              (((uint64_t) 1) << 61)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_IP_NUMBER_VALID           (((uint64_t) 1) << 62)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CONN_PARTY_IP_NUM_VALID           (((uint64_t) 1) << 63)

// all values in set of extended ellaboration should be odd, as need to co-exist with legacy ELA

#define QCRIL_QMI_VOICE_VOIP_CALLINFO_EXT_ELA_STK_CC_EMULATED_OVERLAY    ((((uint64_t) 1) << 1) + 1)
#define QCRIL_QMI_VOICE_VOIP_CALLINFO_EXT_ELA_ALTERNATE_EMERGENCY_CALL   ((((uint64_t) 1) << 2) + 1)

#define VOICE_INVALID_CALL_ID       0xFF
#define VOICE_LOWEST_CALL_ID        1
#define VOICE_HIGHEST_CALL_ID       0xFE

typedef struct {

  uint8_t call_id;

  pi_name_enum_v02 name_pi;
  /**<   Name presentation indicator
       - 0x00 -- PRESENTATION_NAME_ PRESENTATION_ALLOWED -- Allowed presentation
       - 0x01 -- PRESENTATION_NAME_ PRESENTATION_RESTRICTED -- Restricted presentation
       - 0x02 -- PRESENTATION_NAME_UNAVAILABLE -- Unavailable presentation
       - 0x03 -- PRESENTATION_NAME_NAME_ PRESENTATION_RESTRICTED -- Restricted name presentation
   */

  uint32_t name_len;  /**< Must be set to # of elements in name */
  char name[ QCRIL_QMI_VOICE_INTERCODING_BUF_LEN ]; /* in utf8 format */

}voice_remote_party_name_type;  /* Type */

typedef enum
{
    QMI_RIL_VOICE_IMS_EXEC_OVERSIGHT_LINKAGE_NONE,
    QMI_RIL_VOICE_IMS_EXEC_OVERSIGHT_LINKAGE_SPECIFIC_OVERSIGHT_OBJ,
    QMI_RIL_VOICE_IMS_EXEC_OVERSIGHT_LINKAGE_QMI_CALL_ID,
    QMI_RIL_VOICE_IMS_EXEC_OVERSIGHT_LINKAGE_ANDROID_CALL_ID,
    QMI_RIL_VOICE_IMS_EXEC_OVERSIGHT_LINKAGE_ELABORATION_PATTERN
} qmi_ril_voice_ims_command_exec_oversight_call_obj_linkage_e_type;


typedef enum
{
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_NONE,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_AWAITING_RESP_AND_IND,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_RECEIVED_RESP_AWAITING_IND,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_RECEIVED_IND_AWAITING_RESP,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_RECEIVED_IND_AND_RESP_READY_FOR_COMPLETION,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_TIMEOUT_READY_FOR_COMPLETION,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_CALL_ENDED_READY_FOR_COMPLETION,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_RECEIVED_FAILURE_RESP_READY_FOR_COMPLETION,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_STATE_ABANDONED_READY_FOR_COMPLETION
} qmi_ril_voice_ims_command_exec_intermediates_state_e_type;

typedef enum
{
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_NONE,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_COMMENCE_AWAIT_RESP_IND,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_RECEIVED_RESP_SUCCESS,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_RECEIVED_RESP_FAILURE,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_RECEIVED_IND,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_CALL_ENDED,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_TIME_OUT,
    QMI_RIL_VOICE_IMS_EXEC_INTERMED_EVENT_ABANDON
} qmi_ril_voice_ims_command_exec_intermediates_event_e_type;

typedef enum
{
    QMI_RIL_VOICE_IMS_AUDIO_CALL_TYPE_MIN = 0,
    QMI_RIL_VOICE_IMS_AUDIO_CALL_TYPE_VOICE = QMI_RIL_VOICE_IMS_AUDIO_CALL_TYPE_MIN,
    QMI_RIL_VOICE_IMS_AUDIO_CALL_TYPE_IMS,
    QMI_RIL_VOICE_IMS_AUDIO_CALL_TYPE_MAX
} qmi_ril_voice_ims_audio_call_type;

typedef struct
{
   boolean is_add_info_present;
   uint32 total_size;
   uint32 filled_size;
   int8  last_sequence_number;
   uint8* buffer;
} qcril_qmi_voice_additional_call_info_type;

typedef struct qcril_qmi_voice_voip_call_info_entry_type
{
    uint8_t                                 android_call_id;
    uint8_t                                 qmi_call_id;
    int32_t                                 media_id;

    qcril_qmi_voice_voip_call_info_elaboration_type elaboration;
    qcril_qmi_voice_voip_call_info_elaboration_type elaboration_extended;

    voice_call_info2_type_v02               voice_scv_info;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_NUMBER_VALID
    voice_remote_party_number2_type_v02     voice_svc_remote_party_number;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_NAME_VALID
    voice_remote_party_name_type            voice_svc_remote_party_name;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_ALERTING_TYPE_VALID
    voice_alerting_type_type_v02            voice_svc_alerting_type;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CONN_PARTY_NUM_VALID
    voice_conn_num_with_id_type_v02         voice_svc_conn_party_num;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_REDIRECTING_PARTY_NUM_VALID
    voice_num_with_id_type_v02              voice_svc_redirecting_party_num;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_VOICE_PRIVACY_VALID
    voice_privacy_enum_v02                  voice_svc_voice_privacy;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_UUS_VALID
    voice_uus_type_v02                      voice_svc_uus;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_RIL_CALL_STATE_VALID
    RIL_CallState                           ril_call_state;
    // QCRIL_QMI_VOICE_VOIP_RINING_TIME_ID_VALID
    uint32                                  ringing_time_id;
    qcril_qmi_voice_emer_voice_feature_info_type emer_voice_number;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_AUDIO_ATTR_VALID
    voice_call_attributes_type_v02          voice_audio_attrib;   /* Video Call audio attributes */
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_VIDEO_ATTR_VALID
    voice_call_attributes_type_v02          voice_video_attrib;   /* Video Call video attributes */
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_ATTR_VALID
    voice_call_attrib_status_type_v02       call_attrib_status;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_RTT_MODE_VALID
    voice_call_rtt_mode_status_type_v02     rtt_mode;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_RTT_CAP_INFO_VALID
    voice_call_rtt_capabilities_info_type_v02 rtt_capabilities_info;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_IP_NUMBER_VALID
    voice_ip_num_id_type_v02                voice_svc_remote_party_ip_number;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CONN_PARTY_IP_NUM_VALID
    voice_conn_ip_num_with_id_type_v02      voice_svc_conn_party_ip_num;
    uint32                                  call_obj_phase_out_timer_id;
    voice_is_srvcc_call_with_id_type_v02    is_srvcc;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_CHILD_NUMBER_VALID
    voice_child_number_info_type_v02        child_number;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_MODIFY_CONFIRM_PENDING
    qcril::interfaces::CallDomain           to_modify_call_domain;
    qcril::interfaces::CallType             to_modify_call_type;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CODEC_VALID
    voice_speech_codec_enum_v02             codec;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_LOCAL_CALL_CAPBILITIES_VALID
    voice_ip_call_capabilities_info_type_v02 local_call_capabilities_info;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_PEER_CALL_CAPBILITIES_VALID
    voice_ip_call_capabilities_info_type_v02 peer_call_capabilities_info;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_DISPLAY_TEXT_VALID
    voice_display_text_info_type_v02        display_text;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EMER_NUM_TO_IMS_ADDR
    qcril_qmi_voice_emer_num_ims_addr_info_type emer_num_ims_addr_info;
    qmi_ril_voice_ims_audio_call_type       audio_call_type;
    qcril::interfaces::CallType             answered_call_type;
    qcril_qmi_voice_additional_call_info_type additional_call_info;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IP_CALLER_NAME_VALID
    voice_ip_caller_name_info_type_v02        ip_caller_name;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_END_REASON_TEXT_VALID
    voice_ip_end_reason_text_type_v02         end_reason_text;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_CALL_MOD_CAUSE_VALID
    voice_call_modified_cause_enum_v02        call_modified_cause;
    uint8_t                                   is_secure_call;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_IS_CALLED_PARTY_RING_VALID
    voice_volte_is_called_party_ringing_type_v02 is_called_party_ringing;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_HISTORY_INFO_VALID
    voice_ip_hist_info_type_v02               history_info;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_R_PARTY_CONF_CAP_VALID
    voice_remote_party_conf_cap_type_v02      remote_party_conf_capability;
    boolean                                   lcf_valid;
    qcril::interfaces::CallFailCause          lcf;
    call_end_reason_enum_v02                  lcf_extended_codes;
    boolean                                   srvcc_in_progress;
    char*                                     overlayed_number_storage_for_emulated_stk_cc;
    char*                                     overlayed_name_storage_for_emulated_stk_cc;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_SIP_ERROR_CODE_VALID
    voice_ip_end_sip_code_type_v02            sip_error_code;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_ALTERNATE_SIP_URI_VALID
    voice_sip_uri_with_id_type_v02            alternate_sip_uri;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_VERSTAT_INFO_VALID
    voice_MT_call_received_verstat_type_v02   verstat_info;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_TERMINATING_NUM_VALID
    char*                                     terminating_num;
    bool                                      is_secondary;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_EMERG_SRV_CATEG_VALID
    uint32_t                                  emerg_srv_categ;
    // QCRIL_QMI_VOICE_VOIP_CALLINFO_ELA_TIR_MODE_VALID
    sups_mode_enum_v02                        tir_mode;
    struct qcril_qmi_voice_voip_call_info_entry_type * next;
} qcril_qmi_voice_voip_call_info_entry_type;

typedef struct
{
    int nof_voice_calls;
    int nof_voip_calls;
    int nof_active_calls;
    int nof_3gpp_calls;
    int nof_3gpp2_calls;
    int nof_calls_overall;

    qcril_qmi_voice_voip_call_info_entry_type* active_or_single_call;
} qcril_qmi_voice_voip_current_call_summary_type;

void qcril_qmi_voice_voip_lock_overview();
void qcril_qmi_voice_voip_unlock_overview();

qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_create_call_info_entry(
        uint8_t call_qmi_id,
        int32_t call_media_id,
        int need_allocate_call_android_id,
        qcril_qmi_voice_voip_call_info_elaboration_type initial_elaboration,
        RIL_Errno *ril_err );
void qcril_qmi_voice_voip_destroy_call_info_entry( qcril_qmi_voice_voip_call_info_entry_type* entry );
RIL_Errno qcril_qmi_voice_voip_allocate_call_android_id( uint8_t* new_call_android_id );
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_call_qmi_id( uint8_t call_qmi_id );
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_call_android_id( uint8_t call_android_id );
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_elaboration( qcril_qmi_voice_voip_call_info_elaboration_type elaboration_pattern, int pattern_present );
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_elaboration_any_subset( qcril_qmi_voice_voip_call_info_elaboration_type elaboration_pattern );
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_andoid_call_state( RIL_CallState ril_call_state );
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_qmi_call_state(call_state_enum_v02 qmi_call_state);
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_conn_uri( const char *conn_uri );
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_find_call_info_entry_by_single_elaboration_extended( qcril_qmi_voice_voip_call_info_elaboration_type elaboration_single, int single_present );
void qcril_qmi_voice_voip_update_call_info_entry_mainstream (qcril_qmi_voice_voip_call_info_entry_type* entry,
    voice_all_call_status_ind_msg_v02* call_status_ind_ptr,
    uint8_t ril_call_state_valid,
    RIL_CallState ril_call_state
    );
void qcril_qmi_voice_voip_update_call_info_uus(qcril_qmi_voice_voip_call_info_entry_type* entry,
                                               uus_type_enum_v02 uus_type,
                                               uus_dcs_enum_v02 uus_dcs,
                                               uint32_t uus_data_len,
                                               uint8_t *uus_data
                                                 );

int qcril_qmi_voice_voip_call_info_is_single_elaboration_set( const qcril_qmi_voice_voip_call_info_entry_type* entry, qcril_qmi_voice_voip_call_info_elaboration_type single_elaboration );
void qcril_qmi_voice_voip_call_info_set_single_elaboration( qcril_qmi_voice_voip_call_info_entry_type* entry, qcril_qmi_voice_voip_call_info_elaboration_type single_elaboration, int is_set );


void qcril_qmi_voice_voip_mark_all_with(qcril_qmi_voice_voip_call_info_elaboration_type elaboration_set);
void qcril_qmi_voice_voip_unmark_all_with(qcril_qmi_voice_voip_call_info_elaboration_type elaboration_set);
void qcril_qmi_voice_voip_mark_with_specified_call_state(qcril_qmi_voice_voip_call_info_elaboration_type elaboration_set,
                                                         call_state_enum_v02 state);
void qcril_qmi_voice_voip_unmark_with_specified_call_state(qcril_qmi_voice_voip_call_info_elaboration_type elaboration_set,
                                                           call_state_enum_v02 state);

boolean qcril_qmi_voice_voip_call_info_entries_is_empty(void);

qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_call_info_entries_enum_first(void);
qcril_qmi_voice_voip_call_info_entry_type* qcril_qmi_voice_voip_call_info_entries_enum_next(void);

void qcril_qmi_voice_voip_generate_summary( qcril_qmi_voice_voip_current_call_summary_type * summary );

void qcril_qmi_voice_voip_call_info_dump(const qcril_qmi_voice_voip_call_info_entry_type *const call_info_entry);


RIL_Errno qcril_qmi_voice_init ( void );

void qcril_qmi_voice_cleanup( void );

RIL_Errno qcril_qmi_voice_pre_init(void);

uint8_t qcril_qmi_voice_is_set_all_call_forward_supported();

void qcril_qmi_voice_unsol_ind_cb
(
  qmi_client_type                user_handle,
  unsigned int                   msg_id,
  void                          *ind_buf,
  unsigned int                   ind_buf_len,
  void                          *ind_cb_data
);

int qcril_qmi_voice_is_stk_cc_in_progress(void);
void qcril_qmi_voice_reset_stk_cc(void);
void qcril_qmi_voice_stk_cc_relay_alpha_if_necessary(qcril_instance_id_e_type instance_id, boolean send_unsol_unconditional);
void qcril_qmi_voice_stk_cc_handle_voice_sups_ind(voice_sups_ind_msg_v02* sups_ind_msg);
void qcril_qmi_voice_stk_cc_dump(void);

void qcril_qmi_voice_last_call_failure_updated_handler();

void qcril_qmi_voice_nas_control_process_calls_pending_for_right_voice_rte(void *);
void qcril_qmi_voice_nas_control_process_calls_pending_for_specified_voice_rte(unsigned int call_radio_tech_family);

void qcril_qmi_voice_ims_send_unsol_radio_state_change();

void qcril_qmi_voice_ims_client_connected();

unsigned int qcril_qmi_voice_nas_control_convert_radio_tech_to_radio_tech_family(unsigned int voice_radio_tech);

void qcril_qmi_voice_unsol_ind_cb_helper
(
  unsigned int   msg_id,
  unsigned char *decoded_payload,
  uint32_t       decoded_payload_len
);

bool qcril_qmi_voice_match_modem_call_type
(
   call_type_enum_v02                    call_type1,
   bool                                  audio_attrib_valid1,
   voice_call_attribute_type_mask_v02    audio_attrib1,
   bool                                  video_attrib_valid1,
   voice_call_attribute_type_mask_v02    video_attrib1,
   bool                                  rtt_mode_valid1,
   rtt_mode_type_v02                     rtt_mode1,
   call_type_enum_v02                    call_type2,
   bool                                  audio_attrib_valid2,
   voice_call_attribute_type_mask_v02    audio_attrib2,
   bool                                  video_attrib_valid2,
   voice_call_attribute_type_mask_v02    video_attrib2,
   bool                                  rtt_mode_valid2,
   rtt_mode_type_v02                     rtt_mode2
);

int qmi_ril_voice_is_under_any_voice_calls(void);

int qcril_qmi_voice_reboot_cleanup();

typedef boolean (*qcril_qmi_voice_call_filter)
(
    const qcril_qmi_voice_voip_call_info_entry_type* call_info_entry
);

boolean qcril_qmi_voice_has_specific_call
(
    qcril_qmi_voice_call_filter filter,
    call_mode_enum_v02 *call_mode
);

boolean qcril_qmi_voice_is_call_has_ims_audio
(
    const qcril_qmi_voice_voip_call_info_entry_type * call_info_entry
);

boolean qcril_qmi_voice_is_call_has_voice_audio
(
    const qcril_qmi_voice_voip_call_info_entry_type * call_info_entry
);

boolean qcril_qmi_voice_nas_control_is_any_calls_present();

void qcril_qmi_voice_call_info_entry_set_call_modified_cause
(
 qcril_qmi_voice_voip_call_info_entry_type *entry,
 voice_call_modified_cause_enum_v02         call_modified_cause
);

void qcril_qmi_voice_enable_voice_indications
(
  boolean enable
);

void qcril_qmi_voice_dialog_info_ind_hdlr
(
   void *ind_data_ptr,
   uint32 ind_data_len
);

void qcril_qmi_voice_process_qmi_info_ind(uint32_t sequence,
                                          uint8_t total_size_valid,
                                          uint32_t total_size,
                                          uint32_t xml_len,
                                          uint8_t* xml,
                                          uint8_t call_id_valid,
                                          uint8_t call_id,
                                          qcril_qmi_voice_info_ind_xml_type_e_type xml_type);

void qcril_qmi_voice_send_vice_dialog_info_unsol(uint8 *buffer, uint32 size);

void qcril_qmi_voice_ims_send_unsol_vice_dialog_refresh_info_helper(void);

call_mode_enum_v02 qcril_qmi_voice_get_answer_call_mode();

void qcril_qmi_voice_update_ussd_power_opt_buffer(voice_ussd_ind_msg_v02 *ind_data_ptr);
void qcril_qmi_voice_reset_ussd_power_opt_buffer();
void qcril_qmi_voice_notify_ussd_power_opt_to_atel();

void qcril_qmi_voice_ussd_ind_hdlr
(
const void *ind_data_ptr,
uint32 ind_data_len
);

boolean qcril_qmi_voice_call_to_atel(const qcril_qmi_voice_voip_call_info_entry_type * call_info_entry);

void qcril_qmi_voice_all_call_status_ind_hdlr
(
  void *ind_data_ptr,
  uint32 ind_data_len
);

void qcril_qmi_voice_trigger_possible_pending_emergency_call();

void qcril_qmi_voice_request_set_tty_mode_wrapper
(
  void* cb_data
);

void qcril_voice_query_tty_mode_wrapper
(
  void *cb_data
);

void qcril_qmi_voice_request_query_preferred_voice_privacy_mode_wrapper
(
  void *cb_data
);

#ifdef QMI_RIL_UTF
void qcril_qmi_hal_voice_module_cleanup();
#endif

void qcril_qmi_voice_waiting_call_handler
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_voice_am_lch_complete_handler
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_voice_request_set_local_call_hold
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr
);

void qcril_qmi_voice_request_last_call_fail_cause();

void qcril_qmi_voice_send_ims_unsol_call_state_changed();



//===================================================================================

typedef struct {
  uint16_t req_id;
  RIL_Errno errNo;
  void *respData;
} CommonVoiceResponseData;

using CommonVoiceResponseCallback = std::function<void(CommonVoiceResponseData *data)>;

typedef struct
{
    qmi_ril_voice_ims_command_exec_oversight_call_obj_linkage_e_type linkage_type;
    union
    {
        qcril_qmi_voice_voip_call_info_elaboration_type     elaboration_pattern;
        int                                                 qmi_call_id;
        int                                                 android_call_id;
    } linkage;

    qmi_ril_voice_ims_command_exec_intermediates_state_e_type   exec_state;
    call_state_enum_v02                                         target_call_state;
} qmi_ril_voice_ims_command_exec_oversight_link_type;

#define QMI_RIL_VOICE_IMS_EXEC_OVERSIGHT_MAX_NOF_CALLS                                    8

typedef struct qmi_ril_voice_ims_command_exec_oversight_type
{
    uint16_t                                           token;
    qcril_evt_e_type                                   android_request_id;

    uint32                                             timeout_control_timer_id;

    qmi_ril_voice_ims_command_exec_oversight_link_type impacted[ QMI_RIL_VOICE_IMS_EXEC_OVERSIGHT_MAX_NOF_CALLS ] ;
    int                                                nof_impacted;

    uint32                                             successful_response_payload;
    int                                                successful_response_payload_len;

    CommonVoiceResponseCallback                        completion_action;

    struct qmi_ril_voice_ims_command_exec_oversight_type  *next;
} qmi_ril_voice_ims_command_exec_oversight_type;

typedef struct
{
    union
    {
        qmi_ril_voice_ims_command_exec_oversight_type*      command_oversight;
        int                                                 android_call_id;
        int                                                 qmi_call_id;
        qcril_qmi_voice_voip_call_info_elaboration_type     elaboration_pattern;
    } locator;
    call_state_enum_v02                                     new_call_state;
    uint32                                                  successful_response_payload;
    int                                                     successful_response_payload_len;
} qmi_ril_voice_ims_command_exec_oversight_handle_event_params_type;

typedef struct
{
    qcril_qmi_voice_voip_call_info_entry_type * call_info_root;
    qcril_qmi_voice_voip_call_info_entry_type * call_info_enumeration_current;

    qmi_ril_voice_ims_command_exec_oversight_type*
                                                command_exec_oversight_root;

    qmi_ril_voice_ims_command_exec_oversight_type*
                                                command_exec_oversight_current;

    qtimutex::QtiRecursiveMutex                 overview_lock_mutex;

    uint32                                      num_1x_wait_timer_id;
    boolean                                     num_1x_wait_timer_wakelock_is_acquired;
    uint32                                      auto_answer_timer_id;
} qcril_qmi_voice_voip_overview_type;

struct qcril_qmi_pending_call_info_type {
  uint16_t req_id;
  voice_dial_call_req_msg_v02* dial_call_req;
  CommonVoiceResponseCallback responseCb;
  CommonVoiceResponseCallback commandOversightCompletionHandler;
  qcril_qmi_voice_voip_call_info_elaboration_type elaboration;

  qcril_qmi_pending_call_info_type(uint16_t id, const voice_dial_call_req_msg_v02& req,
                                   CommonVoiceResponseCallback callback,
                                   CommonVoiceResponseCallback cmdOversightCallback,
                                   qcril_qmi_voice_voip_call_info_elaboration_type elab)
      : req_id(id),
        responseCb(callback),
        commandOversightCompletionHandler(cmdOversightCallback),
        elaboration(elab) {
    dial_call_req = new voice_dial_call_req_msg_v02;
    if (dial_call_req) {
      *dial_call_req = req;
    }
  }
  qcril_qmi_pending_call_info_type(const qcril_qmi_pending_call_info_type& in)
      : req_id(in.req_id),
        responseCb(in.responseCb),
        commandOversightCompletionHandler(in.commandOversightCompletionHandler),
        elaboration(in.elaboration) {
    if (in.dial_call_req) {
      dial_call_req = new voice_dial_call_req_msg_v02;
      if (dial_call_req) {
        *dial_call_req = *in.dial_call_req;
      }
    } else {
      dial_call_req = nullptr;
    }
  }
  ~qcril_qmi_pending_call_info_type() {
    if (dial_call_req) {
      delete dial_call_req;
    }
  }
};

typedef struct
{
    boolean is_valid;
    RIL_Token token;
    CommonVoiceResponseCallback responseCb;
} qcril_qmi_pending_setup_call_hangup_info_type;

RIL_Errno qcril_qmi_voice_process_manage_calls_req
(
 uint16_t req_id,
 voice_manage_calls_req_msg_v02 &manage_calls_req,
 CommonVoiceResponseCallback responseCb,
 CommonVoiceResponseCallback commandOversightCompletionHandler
);

RIL_Errno qcril_qmi_voice_process_manage_ip_calls_req
(
 uint16_t req_id,
 voice_manage_ip_calls_req_msg_v02 &manage_ip_calls_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_set_call_barring_password_req
(
 uint16_t req_id,
 voice_set_call_barring_password_req_msg_v02 &set_cb_pwd_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_get_call_waiting_req
(
 uint16_t req_id,
 voice_get_call_waiting_req_msg_v02 &get_cw_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_get_colr_req
(
 uint16_t req_id,
 voice_get_colr_req_msg_v02 &colr_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_get_call_forwarding_req
(
 uint16_t req_id,
 voice_get_call_forwarding_req_msg_v02 &get_cf_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_set_sups_service_req
(
 uint16_t req_id,
 voice_set_sups_service_req_msg_v02 &set_sups_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_set_all_call_fwd_sups_req
(
 uint16_t req_id,
 voice_set_all_call_fwd_sups_req_msg_v02 &set_all_call_fwd_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_set_config_req_sync
(
 uint16_t req_id,
 voice_set_config_req_msg_v02 &set_config_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_get_config_req
(
 uint16_t req_id,
 voice_get_config_req_msg_v02 &get_config_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_set_preferred_privacy_req
(
 uint16_t req_id,
 voice_set_preferred_privacy_req_msg_v02 &set_preferred_privacy_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_get_clir_req
(
 uint16_t req_id,
 voice_get_clir_req_msg_v02 &clir_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_set_clir_req
(
 uint32_t clir_n_param
);

RIL_Errno qcril_qmi_voice_process_get_clip_req
(
 uint16_t req_id,
 voice_get_clip_req_msg_v02 &clip_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_get_colp_req
(
 uint16_t req_id,
 voice_get_colp_req_msg_v02 &colp_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_get_call_barring_req
(
 uint16_t req_id,
 voice_get_call_barring_req_msg_v02 &get_cb_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_indication_register_req
(
 uint16_t req_id,
 voice_indication_register_req_msg_v02 &indication_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_ims_call_cancel_req
(
 uint16_t req_id,
 voice_ims_call_cancel_req_msg_v02 &call_cancel_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_send_flash_req
(
 uint16_t req_id,
 voice_send_flash_req_msg_v02 &qmi_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_burst_dtmf_req
(
 uint16_t req_id,
 voice_burst_dtmf_req_msg_v02 &burst_dtmf_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_dial_call_req
(
 uint16_t req_id,
 voice_dial_call_req_msg_v02 &dial_call_req,
 CommonVoiceResponseCallback responseCb,
 CommonVoiceResponseCallback commandOversightCompletionHandler,
 qcril_qmi_voice_voip_call_info_elaboration_type elaboration
);

RIL_Errno qcril_qmi_voice_process_answer_call_req
(
 uint16_t req_id,
 voice_answer_call_req_msg_v02 &ans_call_req,
 CommonVoiceResponseCallback responseCb,
 CommonVoiceResponseCallback commandOversightCompletionHandler
);

RIL_Errno qcril_qmi_voice_process_setup_answer_req
(
 uint16_t req_id,
 voice_setup_answer_req_msg_v02 &setup_answer_req,
 CommonVoiceResponseCallback responseCb,
 CommonVoiceResponseCallback commandOversightCompletionHandler
);

RIL_Errno qcril_qmi_voice_process_end_call_req
(
 uint16_t req_id,
 voice_end_call_req_msg_v02 &end_call_req,
 CommonVoiceResponseCallback responseCb,
 CommonVoiceResponseCallback commandOversightCompletionHandler
);

RIL_Errno qcril_qmi_voice_process_start_cont_dtmf_req
(
 uint16_t req_id,
 voice_start_cont_dtmf_req_msg_v02 &start_cont_dtmf_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_stop_cont_dtmf_req
(
 uint16_t req_id,
 voice_stop_cont_dtmf_req_msg_v02 &stop_cont_dtmf_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_orig_ussd_req
(
 uint16_t req_id,
 voice_orig_ussd_req_msg_v02 &orig_ussd_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_answer_ussd_req
(
 uint16_t req_id,
 voice_answer_ussd_req_msg_v02 &answer_ussd_req,
 CommonVoiceResponseCallback responseCb
);

RIL_Errno qcril_qmi_voice_process_cancel_ussd_req
(
 uint16_t req_id,
 voice_cancel_ussd_req_msg_v02 &cancel_ussd_req,
 CommonVoiceResponseCallback responseCb
);

void qcril_qmi_voice_request_get_current_atel_calls
(
 std::shared_ptr<QcRilRequestGetCurrentCallsMessage> msg
);

void qcril_qmi_voice_get_current_setup_calls
(
 std::shared_ptr<QcRilRequestGetCurrentSetupCallsMessage> msg
);

uint8_t convert_dial_call_req_clir_type(const int &in, clir_type_enum_v02 &out);
uint8_t convert_dial_call_req_uss(const qcril::interfaces::UusInfo &in, voice_uus_type_v02 &out);
RIL_Errno convert_dial_call_req_number(const std::string &number, std::string &calling_number,
                                       std::string &sip_uri_overflow, std::string &sub_address);
uint32_t qcril_qmi_voice_convert_qmi_to_ril_call_forwarding_info(
    int reason,
    uint32_t qmi_call_forwarding_info_len,
    voice_get_call_forwarding_info_type_v02 *qmi_call_forwarding_info,
    uint32_t ril_call_forwarding_info_len,
    RIL_CallForwardInfo *ril_call_forwarding_info);
bool qcril_qmi_voice_map_ril_reason_to_qmi_cfw_reason(voice_reason_enum_v02 &qmi_reason, int ril_reason);
bool qcril_qmi_voice_map_ril_status_to_qmi_cfw_status(voice_service_enum_v02 &out, uint32_t in);
boolean qcril_qmi_voice_map_qmi_to_ril_provision_status
(
 provision_status_enum_v02 qmi_provision_status,
 int *ril_provision_status
);
uint8_t convert_presentation_to_qmi(const qcril::interfaces::Presentation &in, ip_pi_enum_v02 &out);
uint8_t convert_rtt_mode_to_qmi(const qcril::interfaces::RttMode &in, rtt_mode_type_v02 &out);
uint8_t convert_call_mode_to_qmi(const RIL_RadioTechnology &in, call_mode_enum_v02 &out);
uint8_t convert_call_fail_reason_to_qmi(const qcril::interfaces::CallFailCause &in, call_end_reason_enum_v02 &out);
uint8_t convert_call_info_to_qmi(
   const qcril::interfaces::CallType   ril_callType,
   const qcril::interfaces::CallDomain ril_callDomain,
   const bool                          is_emergency_ip,
   call_type_enum_v02                 &call_type,
   uint8_t                            &audio_attrib_valid,
   voice_call_attribute_type_mask_v02 &audio_attrib,
   uint8_t                            &video_attrib_valid,
   voice_call_attribute_type_mask_v02 &video_attrib);
bool qcril_qmi_ims_map_ims_failcause_qmi
(
 const qcril::interfaces::CallFailCause ril_failcause,
 voice_reject_cause_enum_v02 &qmi_failcause
);
bool qcril_qmi_voice_map_ril_ect_type_to_qmi_ect_type(voip_ect_type_enum_v02 &out, qcril::interfaces::EctType in);
bool qcril_qmi_voice_map_ril_tty_mode_to_qmi_tty_mode(tty_mode_enum_v02 &out, qcril::interfaces::TtyMode in);
boolean qcril_qmi_voice_translate_ril_callfwdtimerinfo_to_voice_time_type(
    voice_time_type_v02 &out, const std::shared_ptr<qcril::interfaces::CallFwdTimerInfo> in);
boolean qcril_qmi_voice_translate_voice_time_type_to_ril_callfwdtimerinfo(
    std::shared_ptr<qcril::interfaces::CallFwdTimerInfo> out, const voice_time_type_v02 &in);

std::shared_ptr<qcril::interfaces::SipErrorInfo> buildSipErrorInfo(
    uint8_t sipErrorCodeValid, uint16_t sipErrorCode, uint32_t failureCauseDescriptionLen,
    uint16_t *failureCauseDescription);
qcril::interfaces::ServiceClassStatus mapActiveStatus(active_status_enum_v02 in);
qcril::interfaces::ServiceClassProvisionStatus mapProvisionStatus(provision_status_enum_v02 in);
qcril::interfaces::ClipStatus mapProvisionStatusToClipStatus(provision_status_enum_v02 in);
qcril::interfaces::Presentation mapColrPi(pi_colr_enum_v02 in);
bool mapFacilityType(voice_reason_enum_v02 &out, qcril::interfaces::FacilityType in);
bool mapFacilityType(qcril::interfaces::FacilityType &out, voice_reason_enum_v02 in);
std::shared_ptr<qcril::interfaces::ColrInfo> buildColrInfo(
    const voice_get_colr_resp_msg_v02 *const qmiResp);

qmi_ril_voice_ims_command_exec_oversight_type* qmi_ril_voice_ims_create_command_oversight( uint16_t token,  qcril_evt_e_type android_request_id, int launch_timout_control, CommonVoiceResponseCallback commandOversightCompletionHandler = nullptr );
void qmi_ril_voice_ims_destroy_command_oversight( qmi_ril_voice_ims_command_exec_oversight_type* command_oversight );
qmi_ril_voice_ims_command_exec_oversight_type* qmi_ril_voice_ims_command_oversight_first();
qmi_ril_voice_ims_command_exec_oversight_type* qmi_ril_voice_ims_command_oversight_next();
void qmi_ril_voice_ims_command_oversight_process_completion_candidates( void );
void qmi_ril_voice_ims_command_oversight_dump( qmi_ril_voice_ims_command_exec_oversight_type* command_oversight );
void qmi_ril_voice_ims_command_oversight_add_call_link(  qmi_ril_voice_ims_command_exec_oversight_type* command_oversight,
                                                         qmi_ril_voice_ims_command_exec_oversight_call_obj_linkage_e_type link_type,
                                                         qcril_qmi_voice_voip_call_info_elaboration_type elaboration_pattern,
                                                         int                                             call_id,
                                                         call_state_enum_v02                             target_call_state
                                                       );

int qmi_ril_voice_ims_command_oversight_handle_event(   qmi_ril_voice_ims_command_exec_intermediates_event_e_type event,
                                                         qmi_ril_voice_ims_command_exec_oversight_call_obj_linkage_e_type link_type,
                                                         qmi_ril_voice_ims_command_exec_oversight_handle_event_params_type* params
                                                       );

qmi_ril_voice_ims_command_exec_oversight_type* qmi_ril_voice_ims_find_command_oversight_by_token( uint16_t token );

void qcril_qmi_send_ss_failure_cause_oem_hook_unsol_resp
(
   qmi_sups_errors_enum_v02 sups_failure_cause,
   uint8_t call_id
);

int qcril_qmi_voice_get_cached_clir();
uint8 qcril_qmi_voice_get_ussd_user_action_required();
RIL_Errno qcril_qmi_process_hangup_on_call_being_setup(int *conn_index);
boolean qcril_qmi_voice_cancel_pending_call(bool send_response);
call_composer_importance_enum_v02 convert_priority_to_qmi(const qcril::interfaces::Priority &in);


#endif /* QCRIL_QMI_VOICE_H */
