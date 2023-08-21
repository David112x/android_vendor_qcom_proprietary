#ifndef NAS_EXT_SERVICE_01_H
#define NAS_EXT_SERVICE_01_H
/**
  @file network_access_service_ext_v01.h

  @brief This is the public header file which defines the nas_ext service Data structures.

  This header file defines the types and structures that were defined in
  nas_ext. It contains the constant values defined, enums, structures,
  messages, and service message IDs (in that order) Structures that were
  defined in the IDL as messages contain mandatory elements, optional
  elements, a combination of mandatory and optional elements (mandatory
  always come before optionals in the structure), or nothing (null message)

  An optional element in a message is preceded by a uint8_t value that must be
  set to true if the element is going to be included. When decoding a received
  message, the uint8_t values will be set to true or false by the decode
  routine, and should be checked before accessing the values that they
  correspond to.

  Variable sized arrays are defined as static sized arrays with an unsigned
  integer (32 bit) preceding it that must be set to the number of elements
  in the array that are valid. For Example:

  uint32_t test_opaque_len;
  uint8_t test_opaque[16];

  If only 4 elements are added to test_opaque[] then test_opaque_len must be
  set to 4 before sending the message.  When decoding, the _len value is set
  by the decode routine and should be checked so that the correct number of
  elements in the array will be accessed.

*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2013-2019 Qualcomm Technologies, Inc.
  All rights reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.


  $Header: //commercial/MPSS.HI.1.0.r30/Main/modem_proc/qmimsgs/nas_ext/api/network_access_service_ext_v01.h#1 $
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 6.14.9 
   It was generated on: Wed Jul 31 2019 (Spin 0)
   From IDL File: network_access_service_ext_v01.idl */

/** @defgroup nas_ext_qmi_consts Constant values defined in the IDL */
/** @defgroup nas_ext_qmi_msg_ids Constant values for QMI message IDs */
/** @defgroup nas_ext_qmi_enums Enumerated types used in QMI messages */
/** @defgroup nas_ext_qmi_messages Structures sent as QMI messages */
/** @defgroup nas_ext_qmi_aggregates Aggregate types used in QMI messages */
/** @defgroup nas_ext_qmi_accessor Accessor for QMI service object */
/** @defgroup nas_ext_qmi_version Constant values for versioning information */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "common_v01.h"
#include "network_access_service_common_v01.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup nas_ext_qmi_version
    @{
  */
/** Major Version Number of the IDL used to generate this file */
#define NAS_EXT_V01_IDL_MAJOR_VERS 0x01
/** Revision Number of the IDL used to generate this file */
#define NAS_EXT_V01_IDL_MINOR_VERS 0x15
/** Major Version Number of the qmi_idl_compiler used to generate this file */
#define NAS_EXT_V01_IDL_TOOL_VERS 0x06
/** Maximum Defined Message ID */
#define NAS_EXT_V01_MAX_MESSAGE_ID 0x1002
/**
    @}
  */


/** @addtogroup nas_ext_qmi_consts
    @{
  */
#define QMI_NAS_IMS_DEREGISTRATION_REQ_MSG_V01 0x0087
#define QMI_NAS_IMS_DEREGISTRATION_RESP_MSG_V01 0x0087
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_SET_E911_STATE_ENUM_TYPE_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_E911_ENTER_V01 = 0, /**<  Enter the E911 state. \n  */
  NAS_E911_EXIT_WITH_ECB_V01 = 1, /**<  Exit the E911 state but stay in Emergency Callback mode. \n  */
  NAS_E911_EXIT_V01 = 2, /**<  Exit the E911 state and do not stay in Emergency Callback mode. 
       If already in Emergency Callback mode, exit the mode.  */
  NAS_E911_ENTER_ECB_V01 = 3, /**<  Enter the E911 state with Emergency Callback mode.  */
  NAS_SET_E911_STATE_ENUM_TYPE_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_set_e911_state_enum_type_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_EMERG_TYPE_ENUM_TYPE_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_EMERG_TYPE_CALL_V01 = 0, /**<  Regular emergency calls\n  */
  NAS_EMERG_TYPE_TEXT_V01 = 1, /**<  Emergency text   */
  NAS_EMERG_TYPE_ENUM_TYPE_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_emerg_type_enum_type_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_ELENL_INFO_TYPE_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_ELENL_INFO_PLMN_FULL_V01 = 0, /**<  Dialed number is the extended emergency number, and eenlv is PLMN \n  */
  NAS_ELENL_INFO_MCC_FULL_V01 = 1, /**<  Dialed number is the extended emergency number, and eenlv is MCC 
       FULL acquisition requested   */
  NAS_ELENL_INFO_MCC_LIMITED_V01 = 2, /**<  Dialed number is the extended emergency number, and eenlv is MCC
       limited acquisition requested  */
  NAS_ELENL_INFO_TYPE_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_elenl_info_type_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Commands the modem to enter or exit the E911 state and 
              Emergency Callback mode. */
typedef struct {

  /* Mandatory */
  /*  E911 State Action */
  nas_set_e911_state_enum_type_v01 action;
  /**<   Action to be performed. Values: \n 
      - NAS_E911_ENTER (0) --  Enter the E911 state. \n 
      - NAS_E911_EXIT_WITH_ECB (1) --  Exit the E911 state but stay in Emergency Callback mode. \n 
      - NAS_E911_EXIT (2) --  Exit the E911 state and do not stay in Emergency Callback mode. 
       If already in Emergency Callback mode, exit the mode. 
      - NAS_E911_ENTER_ECB (3) --  Enter the E911 state with Emergency Callback mode. 
 */

  /* Optional */
  /*  Redial */
  uint8_t redial_valid;  /**< Must be set to true if redial is being passed */
  uint8_t redial;
  /**<   Redial status. Values: \n
       - 0 -- No redial \n
       - 1 -- Redial
  */

  /* Optional */
  /*  Emergency Mode Preference */
  uint8_t emerg_mode_pref_valid;  /**< Must be set to true if emerg_mode_pref is being passed */
  mode_pref_mask_type_v01 emerg_mode_pref;
  /**<   Bitmask representing the emergency mode preference to be used.
 Values: \n
      - QMI_NAS_RAT_MODE_PREF_CDMA2000_1X (0x01) --  CDMA2000 1X 
      - QMI_NAS_RAT_MODE_PREF_CDMA2000_HRPD (0x02) --  CDMA2000 HRPD 
      - QMI_NAS_RAT_MODE_PREF_GSM (0x04) --  GSM 
      - QMI_NAS_RAT_MODE_PREF_UMTS (0x08) --  UMTS 
      - QMI_NAS_RAT_MODE_PREF_LTE (0x10) --  LTE
      - QMI_NAS_RAT_MODE_PREF_TDSCDMA (0x20) --  TDSCDMA
      - QMI_NAS_RAT_MODE_PREF_NR5G (0x40) --  NR5G 
 
 \vspace{3pt}
 All the unlisted bits are reserved for future use and the service point
 ignores them if used.
 */

  /* Optional */
  /*  Extended Local Emergency Number Information */
  uint8_t elenl_info_valid;  /**< Must be set to true if elenl_info is being passed */
  nas_elenl_info_type_v01 elenl_info;
  /**<   ELENL call information to be enforced. 
 Valid only if the dialed number is the extended local emergency number.
 Values: \n 
      - NAS_ELENL_INFO_PLMN_FULL (0) --  Dialed number is the extended emergency number, and eenlv is PLMN \n 
      - NAS_ELENL_INFO_MCC_FULL (1) --  Dialed number is the extended emergency number, and eenlv is MCC 
       FULL acquisition requested  
      - NAS_ELENL_INFO_MCC_LIMITED (2) --  Dialed number is the extended emergency number, and eenlv is MCC
       limited acquisition requested 
 */

  /* Optional */
  /*  Emergency Type */
  uint8_t emerg_type_valid;  /**< Must be set to true if emerg_type is being passed */
  nas_emerg_type_enum_type_v01 emerg_type;
}nas_set_e911_state_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Commands the modem to enter or exit the E911 state and 
              Emergency Callback mode. */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. Contains the following data members:
     - qmi_result_type -- QMI_RESULT_SUCCESS or QMI_RESULT_FAILURE \n
     - qmi_error_type  -- Error code. Possible error code values are described in
                          the error codes section of each message definition.
  */

  /* Optional */
  /*  Emergency type */
  uint8_t emerg_type_valid;  /**< Must be set to true if emerg_type is being passed */
  nas_emerg_type_enum_type_v01 emerg_type;
}nas_set_e911_state_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Reports that the UE has found service for the E911 call.  */
typedef struct {

  /* Optional */
  /*  Emergency Ready Indication */
  uint8_t is_ready_due_to_csfb_valid;  /**< Must be set to true if is_ready_due_to_csfb is being passed */
  uint8_t is_ready_due_to_csfb;
  /**<   Indicates whether emergency ready on LTE is due to CSFB. Values: \n
       - TRUE  -- Ready indication is due to CSFB support \n
       - FALSE -- Ready indication is not due to CSFB support \n
	    If this TLV is present and is_ready_due_to_csfb is TRUE, the third party client should dial a CS call to the modem. \n
        If this TLV is present and is_ready_due_to_csfb is FALSE, the third party client should place the
        call over IMS LTE. \n
	    If this TLV is not present and the UE is camped on LTE, the third party client can place the
        call over IMS LTE. 
  */

  /* Optional */
  /*  Emergency type */
  uint8_t emerg_type_valid;  /**< Must be set to true if emerg_type is being passed */
  nas_emerg_type_enum_type_v01 emerg_type;
}nas_e911_state_ready_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_EXT_PH_CMD_TYPE_ENUM_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_PH_CMD_SUBSCRIPTION_NOT_AVAILABLE_V01 = 0x00, /**<  Subscription is not available  */
  NAS_EXT_PH_CMD_TYPE_ENUM_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_ext_ph_cmd_type_enum_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_aggregates
    @{
  */
typedef struct {

  nas_subs_type_enum_v01 subs_type;
  /**<   Subscription type. Values: \n
      - NAS_PRIMARY_SUBSCRIPTION (0x00) --  Primary subscription \n 
      - NAS_SECONDARY_SUBSCRIPTION (0x01) --  Secondary subscription \n 
      - NAS_TERTIARY_SUBSCRIPTION (0x02) --  Tertiary subscription 
 */

  nas_ext_ph_cmd_type_enum_v01 ph_cmd;
  /**<   Subscription status. Values: \n
      - NAS_PH_CMD_SUBSCRIPTION_NOT_AVAILABLE (0x00) --  Subscription is not available 
 */
}nas_subscription_info_type_v01;  /* Type */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Indicates when a subscription starts the refresh process. */
typedef struct {

  /* Optional */
  /*   Subscription Status */
  uint8_t subscription_info_valid;  /**< Must be set to true if subscription_info is being passed */
  nas_subscription_info_type_v01 subscription_info;
}nas_subscription_change_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/**  Message; Indicates when a subscription starts the refresh process. */
typedef struct {
  /* This element is a placeholder to prevent the declaration of
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}nas_ims_deregistration_req_msg_v01;

  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/**  Message; Indicates when a subscription starts the refresh process. */
typedef struct {

  /* Mandatory */
  qmi_response_type_v01 resp;
}nas_ims_deregistration_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Sends notification from the client that IMS has successfully 
              deregistered and a subscription change can proceed. */
typedef struct {
  /* This element is a placeholder to prevent the declaration of
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}nas_ims_proceed_with_subscription_change_req_msg_v01;

  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Sends notification from the client that IMS has successfully 
              deregistered and a subscription change can proceed. */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. Contains the following data members:
     - qmi_result_type -- QMI_RESULT_SUCCESS or QMI_RESULT_FAILURE \n
     - qmi_error_type  -- Error code. Possible error code values are described in
                          the error codes section of each message definition.
  */
}nas_ims_proceed_with_subscription_change_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_T3346_TIMER_STATUS_TYPE_ENUM_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_T3346_TIMER_START_V01 = 0x01, /**<  Timer has started \n  */
  NAS_T3346_TIMER_STOP_V01 = 0x02, /**<  Timer has stopped \n  */
  NAS_T3346_TIMER_EXPIRE_V01 = 0x03, /**<  Timer has expired  */
  NAS_T3346_TIMER_STATUS_TYPE_ENUM_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_t3346_timer_status_type_enum_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Indicates a change in the T3346 timer status. */
typedef struct {

  /* Mandatory */
  /*   T3346 Timer Status */
  nas_t3346_timer_status_type_enum_v01 timer_status;
  /**<   T3346 timer status. Values: \n
      - NAS_T3346_TIMER_START (0x01) --  Timer has started \n 
      - NAS_T3346_TIMER_STOP (0x02) --  Timer has stopped \n 
      - NAS_T3346_TIMER_EXPIRE (0x03) --  Timer has expired 
 */

  /* Optional */
  /*  Radio Access Technology */
  uint8_t radio_access_technology_valid;  /**< Must be set to true if radio_access_technology is being passed */
  nas_radio_if_enum_v01 radio_access_technology;
  /**<   Radio access technology for which to register. Values: \n
       - 0x04 -- RADIO_IF_GSM -- GSM \n
       - 0x05 -- RADIO_IF_UMTS -- UMTS \n 
       - 0x08 -- RADIO_IF_LTE -- LTE \n 
       - 0x0C -- RADIO_IF_NR5G -- NR5G
  */
}nas_t3346_timer_status_change_ind_v01;  /* Message */
/**
    @}
  */

typedef uint64_t nas_ims_call_type_mask_type_v01;
#define NAS_IMS_CALL_TYPE_VOICE_V01 ((nas_ims_call_type_mask_type_v01)0x01ull) /**<  Voice \n  */
#define NAS_IMS_CALL_TYPE_VIDEO_V01 ((nas_ims_call_type_mask_type_v01)0x02ull) /**<  Video \n  */
#define NAS_IMS_CALL_TYPE_SMS_V01 ((nas_ims_call_type_mask_type_v01)0x04ull) /**<  SMS \n  */
#define NAS_IMS_CALL_TYPE_EMERGENCY_V01 ((nas_ims_call_type_mask_type_v01)0x08ull) /**<  Emergency  */
/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_IMS_CALL_STATUS_ENUM_TYPE_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_IMS_CALL_STATUS_START_V01 = 0x00, /**<  Call has started \n  */
  NAS_IMS_CALL_STATUS_STOP_V01 = 0x01, /**<  Call has stopped \n  */
  NAS_IMS_CALL_STATUS_CONNECTED_V01 = 0x02, /**<  Call has connected  */
  NAS_IMS_CALL_STATUS_ENUM_TYPE_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_ims_call_status_enum_type_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_IMS_CALL_DIRECTION_ENUM_TYPE_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_IMS_CALL_DIRECTION_MO_V01 = 0x00, /**<  Mobile-originated call \n  */
  NAS_IMS_CALL_DIRECTION_MT_V01 = 0x01, /**<  Mobile-terminated call  */
  NAS_IMS_CALL_DIRECTION_ENUM_TYPE_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_ims_call_direction_enum_type_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Informs the modem about the MMTEL voice, video, or SMS call 
              states on the control point. */
typedef struct {

  /* Mandatory */
  /*  Call State */
  nas_ims_call_status_enum_type_v01 call_status;
  /**<   Call status. Values: \n 
      - NAS_IMS_CALL_STATUS_START (0x00) --  Call has started \n 
      - NAS_IMS_CALL_STATUS_STOP (0x01) --  Call has stopped \n 
      - NAS_IMS_CALL_STATUS_CONNECTED (0x02) --  Call has connected 
 */

  /* Optional */
  /*  Call Type */
  uint8_t call_type_valid;  /**< Must be set to true if call_type is being passed */
  nas_ims_call_type_mask_type_v01 call_type;
  /**<   Bitmask representing the call type. Values: \n
      - NAS_IMS_CALL_TYPE_VOICE (0x01) --  Voice \n 
      - NAS_IMS_CALL_TYPE_VIDEO (0x02) --  Video \n 
      - NAS_IMS_CALL_TYPE_SMS (0x04) --  SMS \n 
      - NAS_IMS_CALL_TYPE_EMERGENCY (0x08) --  Emergency 

 \vspace{3pt}
 Any combination of the bit positions is valid. 

 \vspace{3pt}
 Without this TLV, the default behavior is for voice call type.
 Only one call type should be set while notifying 
 call_status=NAS_IMS_CALL_STATUS_START.
 */

  /* Optional */
  /*  Sys Mode */
  uint8_t sys_mode_valid;  /**< Must be set to true if sys_mode is being passed */
  nas_radio_if_enum_v01 sys_mode;
  /**<   Radio interface system mode. Values: \n
      - NAS_RADIO_IF_NO_SVC (0x00) --  None (no service) \n 
      - NAS_RADIO_IF_CDMA_1X (0x01) --  cdma2000\textsuperscript{\textregistered} 1X \n 
      - NAS_RADIO_IF_CDMA_1XEVDO (0x02) --  cdma2000\textsuperscript{\textregistered} HRPD (1xEV-DO) \n 
      - NAS_RADIO_IF_AMPS (0x03) --  AMPS \n 
      - NAS_RADIO_IF_GSM (0x04) --  GSM \n 
      - NAS_RADIO_IF_UMTS (0x05) --  UMTS \n 
      - NAS_RADIO_IF_WLAN (0x06) --  WLAN \n 
      - NAS_RADIO_IF_GPS (0x07) --  GPS \n 
      - NAS_RADIO_IF_LTE (0x08) --  LTE \n 
      - NAS_RADIO_IF_TDSCDMA (0x09) --  TD-SCDMA \n 
      - NAS_RADIO_IF_NR5G (0x0C) --  NR5G \n 
      - NAS_RADIO_IF_NO_CHANGE (-1) --  No change 

 \vspace{3pt}
 The only valid system modes for this request are WLAN, LTE, and NR5G.

 \vspace{3pt}
 Without this TLV, the default behavior is for system mode LTE.
 */

  /* Optional */
  /*  Call Direction */
  uint8_t call_direction_valid;  /**< Must be set to true if call_direction is being passed */
  nas_ims_call_direction_enum_type_v01 call_direction;
  /**<   Call Direction. Values: \n 
      - NAS_IMS_CALL_DIRECTION_MO (0x00) --  Mobile-originated call \n 
      - NAS_IMS_CALL_DIRECTION_MT (0x01) --  Mobile-terminated call 

 \vspace{3pt}
 Without this TLV, the default behavior is for a mobile-originated
 call.
 QMI_NAS_MMTEL_RESPONSE_IND is sent only for mobile orignated 
 calls on LTE or NR5G.
 */
}nas_ims_call_state_nofification_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Informs the modem about the MMTEL voice, video, or SMS call 
              states on the control point. */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. Contains the following data members:
     - qmi_result_type -- QMI_RESULT_SUCCESS or QMI_RESULT_FAILURE \n
     - qmi_error_type  -- Error code. Possible error code values are described in
                          the error codes section of each message definition.
  */
}nas_ims_call_state_notification_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_CALL_MODE_STATE_ENUM_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_CALL_MODE_NORMAL_V01 = 0x01, /**<  Normal \n  */
  NAS_CALL_MODE_VOLTE_ONLY_V01 = 0x02, /**<  VoLTE only  */
  NAS_CALL_MODE_STATE_ENUM_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_call_mode_state_enum_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Indicates the call mode state of the modem. */
typedef struct {

  /* Mandatory */
  /*  Call Mode */
  nas_call_mode_state_enum_v01 call_mode;
  /**<   Call mode state. Values: \n
      - NAS_CALL_MODE_NORMAL (0x01) --  Normal \n 
      - NAS_CALL_MODE_VOLTE_ONLY (0x02) --  VoLTE only 
 */
}nas_call_mode_ind_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Retrieves the current call mode status of the modem. */
typedef struct {
  /* This element is a placeholder to prevent the declaration of
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}nas_get_call_mode_req_msg_v01;

  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Retrieves the current call mode status of the modem. */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. Contains the following data members:
     - qmi_result_type -- QMI_RESULT_SUCCESS or QMI_RESULT_FAILURE \n
     - qmi_error_type  -- Error code. Possible error code values are described in
                          the error codes section of each message definition.
  */

  /* Optional */
  /*  Call Mode */
  uint8_t call_mode_valid;  /**< Must be set to true if call_mode is being passed */
  nas_call_mode_state_enum_v01 call_mode;
  /**<   Call mode. Values: \n
      - NAS_CALL_MODE_NORMAL (0x01) --  Normal \n 
      - NAS_CALL_MODE_VOLTE_ONLY (0x02) --  VoLTE only 
 */
}nas_get_call_mode_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Informs the modem about whether VoLTE is enabled or disabled 
              on the control point. (Deprecated) */
typedef struct {

  /* Mandatory */
  /*  Call State */
  uint8_t is_volte_enabled;
  /**<   VoLTE status. Values: \n
       - TRUE  -- Enabled \n
       - FALSE -- Disabled
   */
}nas_volte_state_notification_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Informs the modem about whether VoLTE is enabled or disabled 
              on the control point. (Deprecated) */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. Contains the following data members:
     - qmi_result_type -- QMI_RESULT_SUCCESS or QMI_RESULT_FAILURE \n
     - qmi_error_type  -- Error code. Possible error code values are described in
                          the error codes section of each message definition.
  */
}nas_volte_state_notification_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Reports that the modem has completed one round of emergency scanning. */
typedef struct {

  /* Mandatory */
  /*  Emergency Mode Preference */
  mode_pref_mask_type_v01 emerg_mode_pref;
  /**<   Bitmask representing the emergency mode preference used for scanning.
 Values: \n
      - QMI_NAS_RAT_MODE_PREF_CDMA2000_1X (0x01) --  CDMA2000 1X 
      - QMI_NAS_RAT_MODE_PREF_CDMA2000_HRPD (0x02) --  CDMA2000 HRPD 
      - QMI_NAS_RAT_MODE_PREF_GSM (0x04) --  GSM 
      - QMI_NAS_RAT_MODE_PREF_UMTS (0x08) --  UMTS 
      - QMI_NAS_RAT_MODE_PREF_LTE (0x10) --  LTE
      - QMI_NAS_RAT_MODE_PREF_TDSCDMA (0x20) --  TDSCDMA
      - QMI_NAS_RAT_MODE_PREF_NR5G (0x40) --  NR5G 
 
 \vspace{3pt}
 All the unlisted bits are reserved for future use.
 */

  /* Optional */
  /*  Emergency type */
  uint8_t emerg_type_valid;  /**< Must be set to true if emerg_type is being passed */
  nas_emerg_type_enum_type_v01 emerg_type;
}nas_e911_search_fail_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Notifies the modem whether VoLTE or VoNR has been enabled on the control point.
              \label{idl:voimsStateNotification} */
typedef struct {

  /* Mandatory */
  /*  VoLTE State */
  uint8_t volte_is_enabled;
  /**<   VoLTE status.
       Values: \n
       - TRUE  -- Enabled \n
       - FALSE -- Disabled
   */

  /* Mandatory */
  /*  VoNR State */
  uint8_t vonr_is_enabled;
  /**<   VoNR status.
       Values: \n
       - TRUE  -- Enabled \n
       - FALSE -- Disabled
   */
}nas_voims_state_notification_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Notifies the modem whether VoLTE or VoNR has been enabled on the control point.
              \label{idl:voimsStateNotification} */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. Contains the following data members:
     - qmi_result_type -- QMI_RESULT_SUCCESS or QMI_RESULT_FAILURE \n
     - qmi_error_type  -- Error code. Possible error code values are described in
                          the error codes section of each message definition.
  */
}nas_voims_state_notification_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Commands the modem to scan the requested RATs to perform silent 
	          redial for the MMTEL voice call/video call originated. */
typedef struct {

  /* Mandatory */
  /*  Mode Preference */
  mode_pref_mask_type_v01 mode_pref;
  /**<   Bitmask representing the mode preference to be used for redialing the 
 MMTEL voice/video call originated.
 Values: \n
      - QMI_NAS_RAT_MODE_PREF_CDMA2000_1X (0x01) --  CDMA2000 1X 
      - QMI_NAS_RAT_MODE_PREF_CDMA2000_HRPD (0x02) --  CDMA2000 HRPD 
      - QMI_NAS_RAT_MODE_PREF_GSM (0x04) --  GSM 
      - QMI_NAS_RAT_MODE_PREF_UMTS (0x08) --  UMTS 
      - QMI_NAS_RAT_MODE_PREF_LTE (0x10) --  LTE
      - QMI_NAS_RAT_MODE_PREF_TDSCDMA (0x20) --  TDSCDMA
      - QMI_NAS_RAT_MODE_PREF_NR5G (0x40) --  NR5G 
 
 \vspace{3pt}
 All the unlisted bits are reserved for future use and the service point
 ignores them if used.
 */

  /* Mandatory */
  /*  Call Type */
  nas_ims_call_type_mask_type_v01 call_type;
  /**<   Bitmask representing the type of the call in origination state. Values: \n
       - NAS_IMS_CALL_TYPE_VOICE       = 0x01 -- Voice \n 
       - NAS_IMS_CALL_TYPE_VIDEO       = 0x02 -- Video \n 
 
       \vspace{3pt}
       Any combination of the bit positions is valid. 
   */
}nas_call_pref_change_req_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Commands the modem to scan the requested RATs to perform silent 
	          redial for the MMTEL voice call/video call originated. */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. Contains the following data members:
     - qmi_result_type -- QMI_RESULT_SUCCESS or QMI_RESULT_FAILURE \n
     - qmi_error_type  -- Error code. Possible error code values are described in
                          the error codes section of each message definition.
  */
}nas_call_pref_change_resp_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Reports that the UE has found service to redial the voice call/video call.  */
typedef struct {

  /* Mandatory */
  /*  Sys Mode */
  nas_radio_if_enum_v01 sys_mode;
  /**<   Radio interface system mode available for redialing the voice/video call. Values: \n
      - NAS_RADIO_IF_NO_SVC (0x00) --  None (no service) \n 
      - NAS_RADIO_IF_CDMA_1X (0x01) --  cdma2000\textsuperscript{\textregistered} 1X \n 
      - NAS_RADIO_IF_CDMA_1XEVDO (0x02) --  cdma2000\textsuperscript{\textregistered} HRPD (1xEV-DO) \n 
      - NAS_RADIO_IF_AMPS (0x03) --  AMPS \n 
      - NAS_RADIO_IF_GSM (0x04) --  GSM \n 
      - NAS_RADIO_IF_UMTS (0x05) --  UMTS \n 
      - NAS_RADIO_IF_WLAN (0x06) --  WLAN \n 
      - NAS_RADIO_IF_GPS (0x07) --  GPS \n 
      - NAS_RADIO_IF_LTE (0x08) --  LTE \n 
      - NAS_RADIO_IF_TDSCDMA (0x09) --  TD-SCDMA \n 
      - NAS_RADIO_IF_NR5G (0x0C) --  NR5G \n 
      - NAS_RADIO_IF_NO_CHANGE (-1) --  No change 
 */
}nas_call_ready_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_enums
    @{
  */
typedef enum {
  NAS_MMTEL_RESPONSE_ENUM_TYPE_MIN_ENUM_VAL_V01 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  NAS_MMTEL_SUCCESS_V01 = 1, /**<  Access allowed \n */
  NAS_MMTEL_ACCESS_BARRED_V01 = 2, /**<  Access barred \n */
  NAS_MMTEL_INVALID_RAT_V01 = 3, /**<  UE is in the process of reselecting to another RAT. \n 
                                     Backoff and retry. No UAC check performed \n */
  NAS_MMTEL_INVALID_STATE_V01 = 4, /**<  Modem is in deregistered state; no UAC check performed \n */
  NAS_MMTEL_NO_SERVICE_V01 = 5, /**<  UE is not camped; no UAC check performed \n  */
  NAS_MMTEL_T3346_ACTIVE_V01 = 6, /**<  Timer T3346 is active; no UAC check performed \n  */
  NAS_MMTEL_SERVICE_AREA_RESTRICTION_V01 = 7, /**<  Service area is restricted; no UAC check performed   */
  NAS_MMTEL_RESPONSE_ENUM_TYPE_MAX_ENUM_VAL_V01 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}nas_mmtel_response_enum_type_v01;
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Reports the MMTEL response from the modem. */
typedef struct {

  /* Mandatory */
  /*  MMTEL Response */
  nas_mmtel_response_enum_type_v01 mmtel_response;
  /**<   MMTEL Response. Values: \n
      - NAS_MMTEL_SUCCESS (1) --  Access allowed \n
      - NAS_MMTEL_ACCESS_BARRED (2) --  Access barred \n
      - NAS_MMTEL_INVALID_RAT (3) --  UE is in the process of reselecting to another RAT. \n 
                                     Backoff and retry. No UAC check performed \n
      - NAS_MMTEL_INVALID_STATE (4) --  Modem is in deregistered state; no UAC check performed \n
      - NAS_MMTEL_NO_SERVICE (5) --  UE is not camped; no UAC check performed \n 
      - NAS_MMTEL_T3346_ACTIVE (6) --  Timer T3346 is active; no UAC check performed \n 
      - NAS_MMTEL_SERVICE_AREA_RESTRICTION (7) --  Service area is restricted; no UAC check performed  
 */

  /* Mandatory */
  /*  Call Type */
  nas_ims_call_type_mask_type_v01 call_type;
  /**<   Bitmask representing the call type. This is the same call_type 
       specified in the MMTEL call state indication.\n
       Values: \n
       - NAS_IMS_CALL_TYPE_VOICE       -- 0x01 -- Voice \n 
       - NAS_IMS_CALL_TYPE_VIDEO       -- 0x02 -- Video \n 
       - NAS_IMS_CALL_TYPE_SMS         -- 0x04 --  SMS 

       \vspace{3pt}
       Any combination of the bit positions is valid. 
   */

  /* Mandatory */
  /*  Sys Mode */
  nas_radio_if_enum_v01 sys_mode;
  /**<   Radio interface system mode. This is the same sys_mode 
       specified in the MMTEL call state indication.\n
       Values: \n  
       - NAS_RADIO_IF_LTE        -- 0x08 --LTE \n 
       - NAS_RADIO_IF_NR5G       -- 0x0C --NR5G 
  */

  /* Mandatory */
  /*  Barring Time */
  uint32_t barring_time;
  /**<   Time to back off and retry (in milliseconds).
  */
}nas_mmtel_response_ind_msg_v01;  /* Message */
/**
    @}
  */

typedef uint64_t nas_mmtel_call_type_mask_type_v01;
#define NAS_MMTEL_CALL_TYPE_VOICE_V01 ((nas_mmtel_call_type_mask_type_v01)0x01ull) /**<  Voice \n  */
#define NAS_MMTEL_CALL_TYPE_VIDEO_V01 ((nas_mmtel_call_type_mask_type_v01)0x02ull) /**<  Video \n  */
#define NAS_MMTEL_CALL_TYPE_SMS_V01 ((nas_mmtel_call_type_mask_type_v01)0x04ull) /**<  SMS   */
/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Indication Message; Reports the call types for which UAC barring has been removed.  */
typedef struct {

  /* Mandatory */
  /*  Call Type */
  nas_mmtel_call_type_mask_type_v01 call_type;
  /**<   Bitmask representing the call types for which UAC barring has been removed. Values: \n
      - NAS_MMTEL_CALL_TYPE_VOICE (0x01) --  Voice \n 
      - NAS_MMTEL_CALL_TYPE_VIDEO (0x02) --  Video \n 
      - NAS_MMTEL_CALL_TYPE_SMS (0x04) --  SMS  
 */
}nas_uac_barring_alleviation_ind_msg_v01;  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Request Message; Gets the subscription to place an emergency call. */
typedef struct {
  /* This element is a placeholder to prevent the declaration of
     an empty struct.  DO NOT USE THIS FIELD UNDER ANY CIRCUMSTANCE */
  char __placeholder;
}nas_get_e911_sub_req_msg_v01;

  /* Message */
/**
    @}
  */

/** @addtogroup nas_ext_qmi_messages
    @{
  */
/** Response Message; Gets the subscription to place an emergency call. */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
  /**<   Standard response type. */

  /* Optional */
  /*  E911 Subscription */
  uint8_t e911_sub_valid;  /**< Must be set to true if e911_sub is being passed */
  nas_subs_type_enum_v01 e911_sub;
  /**<   Values: \n
       -0x00 -- Primary subscription \n
       -0x01 -- Secondary subscription 
       \vspace{3pt}
       All other values are reserved.
  */
}nas_get_e911_sub_resp_msg_v01;  /* Message */
/**
    @}
  */

/* Conditional compilation tags for message removal */ 
//#define REMOVE_QMI_NAS_CALL_MODE_IND_V01
//#define REMOVE_QMI_NAS_CALL_PREF_CHANGE_V01
//#define REMOVE_QMI_NAS_CALL_READY_IND_V01
//#define REMOVE_QMI_NAS_E911_SEARCH_FAIL_IND_V01
//#define REMOVE_QMI_NAS_E911_STATE_READY_IND_V01
//#define REMOVE_QMI_NAS_GET_CALL_MODE_V01
//#define REMOVE_QMI_NAS_GET_E911_SUB_V01
//#define REMOVE_QMI_NAS_IMS_CALL_STATE_NOTIFICATION_V01
//#define REMOVE_QMI_NAS_IMS_PROCEED_WITH_SUBSCRIPTION_CHANGE_V01
//#define REMOVE_QMI_NAS_MMTEL_RESPONSE_IND_V01
//#define REMOVE_QMI_NAS_SET_E911_STATE_V01
//#define REMOVE_QMI_NAS_SUBSCRIPTION_CHANGE_IND_V01
//#define REMOVE_QMI_NAS_T3346_TIMER_STATUS_CHANGE_IND_V01
//#define REMOVE_QMI_NAS_UAC_BARRING_ALLEVIATION_IND_V01
//#define REMOVE_QMI_NAS_VOIMS_STATE_NOTIFICATION_V01
//#define REMOVE_QMI_NAS_VOLTE_STATE_NOTIFICATION_V01

/*Service Message Definition*/
/** @addtogroup nas_ext_qmi_msg_ids
    @{
  */
#define QMI_NAS_SET_E911_STATE_REQ_MSG_V01 0x007A
#define QMI_NAS_SET_E911_STATE_RESP_MSG_V01 0x007A
#define QMI_NAS_E911_STATE_READY_IND_V01 0x007B
#define QMI_NAS_SUBSCRIPTION_CHANGE_IND_V01 0x0086
#define QMI_NAS_IMS_PROCEED_WITH_SUBSCRIPTION_CHANGE_REQ_MSG_V01 0x0087
#define QMI_NAS_IMS_PROCEED_WITH_SUBSCRIPTION_CHANGE_RESP_MSG_V01 0x0087
#define QMI_NAS_T3346_TIMER_STATUS_CHANGE_IND_V01 0x00A0
#define QMI_NAS_IMS_CALL_STATE_NOTIFICATION_REQ_MSG_V01 0x00A1
#define QMI_NAS_IMS_CALL_STATE_NOTIFICATION_RESP_MSG_V01 0x00A1
#define QMI_NAS_CALL_MODE_IND_V01 0x00A2
#define QMI_NAS_GET_CALL_MODE_REQ_MSG_V01 0x00A3
#define QMI_NAS_GET_CALL_MODE_RESP_MSG_V01 0x00A3
#define QMI_NAS_VOLTE_STATE_NOTIFICATION_REQ_MSG_V01 0x00A4
#define QMI_NAS_VOLTE_STATE_NOTIFICATION_RESP_MSG_V01 0x00A4
#define QMI_NAS_VOIMS_STATE_NOTIFICATION_REQ_MSG_V01 0x00DD
#define QMI_NAS_VOIMS_STATE_NOTIFICATION_RESP_MSG_V01 0x00DD
#define QMI_NAS_CALL_PREF_CHANGE_REQ_MSG_V01 0x00E2
#define QMI_NAS_CALL_PREF_CHANGE_RESP_MSG_V01 0x00E2
#define QMI_NAS_CALL_READY_IND_V01 0x00E3
#define QMI_NAS_MMTEL_RESPONSE_IND_V01 0x00E4
#define QMI_NAS_UAC_BARRING_ALLEVIATION_IND_V01 0x00E5
#define QMI_NAS_E911_SEARCH_FAIL_IND_V01 0x1001
#define QMI_NAS_GET_E911_SUB_REQ_MSG_V01 0x1002
#define QMI_NAS_GET_E911_SUB_RESP_MSG_V01 0x1002
/**
    @}
  */

/* Service Object Accessor */
/** @addtogroup wms_qmi_accessor
    @{
  */
/** This function is used internally by the autogenerated code.  Clients should use the
   macro nas_ext_get_service_object_v01( ) that takes in no arguments. */
qmi_idl_service_object_type nas_ext_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );

/** This macro should be used to get the service object */
#define nas_ext_get_service_object_v01( ) \
          nas_ext_get_service_object_internal_v01( \
            NAS_EXT_V01_IDL_MAJOR_VERS, NAS_EXT_V01_IDL_MINOR_VERS, \
            NAS_EXT_V01_IDL_TOOL_VERS )
/**
    @}
  */


#ifdef __cplusplus
}
#endif
#endif

