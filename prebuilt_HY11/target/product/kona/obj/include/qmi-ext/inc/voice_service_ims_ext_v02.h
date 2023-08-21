#ifndef VOICE_IMS_EXT_SERVICE_02_H
#define VOICE_IMS_EXT_SERVICE_02_H
/**
  @file voice_service_ims_ext_v02.h

  @brief This is the public header file which defines the voice_ims_ext service Data structures.

  This header file defines the types and structures that were defined in
  voice_ims_ext. It contains the constant values defined, enums, structures,
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
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.


  $Header: //commercial/MPSS.HI.1.0.r30/Main/modem_proc/qmimsgs/voice_ext/api/voice_service_ims_ext_v02.h#1 $
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====* 
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 6.14.4 
   It was generated on: Fri May  1 2015 (Spin 1)
   From IDL File: voice_service_ims_ext_v02.idl */

/** @defgroup voice_ims_ext_qmi_consts Constant values defined in the IDL */
/** @defgroup voice_ims_ext_qmi_msg_ids Constant values for QMI message IDs */
/** @defgroup voice_ims_ext_qmi_enums Enumerated types used in QMI messages */
/** @defgroup voice_ims_ext_qmi_messages Structures sent as QMI messages */
/** @defgroup voice_ims_ext_qmi_aggregates Aggregate types used in QMI messages */
/** @defgroup voice_ims_ext_qmi_accessor Accessor for QMI service object */
/** @defgroup voice_ims_ext_qmi_version Constant values for versioning information */

#include <stdint.h>
#include "qmi_idl_lib.h"
#include "common_v01.h"
#include "voice_service_common_v02.h"


#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup voice_ims_ext_qmi_version
    @{
  */
/** Major Version Number of the IDL used to generate this file */
#define VOICE_IMS_EXT_V02_IDL_MAJOR_VERS 0x02
/** Revision Number of the IDL used to generate this file */
#define VOICE_IMS_EXT_V02_IDL_MINOR_VERS 0x04
/** Major Version Number of the qmi_idl_compiler used to generate this file */
#define VOICE_IMS_EXT_V02_IDL_TOOL_VERS 0x06
/** Maximum Defined Message ID */
#define VOICE_IMS_EXT_V02_MAX_MESSAGE_ID 0x0065
/**
    @}
  */


/** @addtogroup voice_ims_ext_qmi_consts 
    @{ 
  */
#define QMI_VOICE_SRVCC_CALL_CONTEXT_ARRAY_MAX_V02 7
#define QMI_VOICE_NUMBER_MAX_V02 81
#define QMI_VOICE_CALLER_NAME_MAX_V02 182
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_enums
    @{
  */
typedef enum {
  CALL_SUBSTATE_ENUM_MIN_ENUM_VAL_V02 = -2147483647, /**< To force a 32 bit signed enum.  Do not change or use*/
  CALL_SUBSTATE_DEFAULT_V02 = 0x00, /**<  Substate is the same as the call state \n  */
  CALL_SUBSTATE_INCOM_ANSWERED_V02 = 0x01, /**<  Incoming call has been answered and is not yet connected \n  */
  CALL_SUBSTATE_HOLD_REQ_UE_V02 = 0x02, /**<  Hold request is in progress for an active call \n  */
  CALL_SUBSTATE_RETRIVE_REQ_UE_V02 = 0x03, /**<  Retrieve request is in progress for a hold call \n */
  CALL_SUBSTATE_PRE_ALERTING_V02 = 0x04, /**<  Early media is being played before ringing begins  */
  CALL_SUBSTATE_ENUM_MAX_ENUM_VAL_V02 = 2147483647 /**< To force a 32 bit signed enum.  Do not change or use*/
}call_substate_enum_v02;
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t instance_id;
  /**<   Unique identifier for the call set by the client.
  */

  call_type_enum_v02 call_type;
  /**<   Call type. Values: \n
       - 0x00 -- CALL_TYPE_VOICE         -- Voice (automatic selection) \n
       - 0x09 -- CALL_TYPE_EMERGENCY     -- Emergency
    */

  call_state_enum_v02 call_state;
  /**<   Call state. Values: \n       
       - 0x01 -- CALL_STATE_ORIGINATING    -- Origination \n 
       - 0x02 -- CALL_STATE_INCOMING       -- Incoming \n
       - 0x03 -- CALL_STATE_CONVERSATION   -- Conversation \n
       - 0x05 -- CALL_STATE_ALERTING       -- Alerting \n
       - 0x06 -- CALL_STATE_HOLD           -- Hold \n
       - 0x07 -- CALL_STATE_WAITING        -- Waiting
  */

  call_substate_enum_v02 call_substate;
  /**<   Substate of the call. Values: \n
      - CALL_SUBSTATE_DEFAULT (0x00) --  Substate is the same as the call state \n 
      - CALL_SUBSTATE_INCOM_ANSWERED (0x01) --  Incoming call has been answered and is not yet connected \n 
      - CALL_SUBSTATE_HOLD_REQ_UE (0x02) --  Hold request is in progress for an active call \n 
      - CALL_SUBSTATE_RETRIVE_REQ_UE (0x03) --  Retrieve request is in progress for a hold call \n
      - CALL_SUBSTATE_PRE_ALERTING (0x04) --  Early media is being played before ringing begins  
 */

  uint8_t is_mpty;
  /**<   Multiparty indicator. Values: \n
       - 0x00 -- FALSE \n
       - 0x01 -- TRUE
  */

  call_direction_enum_v02 direction;
  /**<   Direction. Values: \n
       - 0x01 -- CALL_DIRECTION_MO -- MO call \n
       - 0x02 -- CALL_DIRECTION_MT -- MT call
  */

  char calling_number[QMI_VOICE_NUMBER_MAX_V02 + 1];
  /**<   Calling number of the call as an ASCII string. 
       Length range: 1 to 81.
  */
}voice_srvcc_call_context_type_v02;  /* Type */
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t instance_id;
  /**<   Unique identifier for the call set by the client.
  */

  pi_name_enum_v02 name_pi;
  /**<   Name presentation indicator. Values: \n
      - PRESENTATION_NAME_PRESENTATION_ALLOWED (0x00) --  Allowed presentation \n 
      - PRESENTATION_NAME_PRESENTATION_RESTRICTED (0x01) --  Restricted presentation \n 
      - PRESENTATION_NAME_UNAVAILABLE (0x02) --  Unavailable presentation \n 
      - PRESENTATION_NAME_NAME_PRESENTATION_RESTRICTED (0x03) --  Restricted name presentation 
 */

  uint32_t caller_name_utf16_len;  /**< Must be set to # of elements in caller_name_utf16 */
  uint16_t caller_name_utf16[QMI_VOICE_CALLER_NAME_MAX_V02];
  /**<   Caller name per UTF-16 coding scheme.
  */
}voice_srvcc_call_context_name_type_v02;  /* Type */
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t instance_id;
  /**<   Unique identifier for the call set by the client.
  */

  pi_num_enum_v02 pi;
  /**<   Presentation indicator. Values: \n
      - PRESENTATION_NUM_ALLOWED (0x00) --  Allowed presentation \n 
      - PRESENTATION_NUM_RESTRICTED (0x01) --  Restricted presentation \n 
      - PRESENTATION_NUM_NUM_UNAVAILABLE (0x02) --  Unavailable presentation \n 
      - PRESENTATION_NUM_RESERVED (0x03) --  Reserved presentation \n 
      - PRESENTATION_NUM_PAYPHONE (0x04) --  Payphone presentation (GSM/UMTS specific) 
 */
}voice_srvcc_call_context_num_pi_type_v02;  /* Type */
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_aggregates
    @{
  */
typedef struct {

  uint8_t instance_id;
  /**<   Unique identifier for the call set by the client.
  */

  alerting_type_enum_v02 alerting_info;
  /**<   Alerting information. Values: \n
       - 0x00 -- ALERTING_LOCAL  -- Network is playing a tone \n
       - 0x01 -- ALERTING_REMOTE -- UE is playing a tone
  */
}voice_srvcc_alerting_info_type_v02;  /* Type */
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_messages
    @{
  */
/** Request Message; Allows third party IMS solutions to provide call information 
	         during SRVCC. */
typedef struct {

  /* Mandatory */
  /*  Array of Call Information  */
  uint32_t srvcc_call_context_len;  /**< Must be set to # of elements in srvcc_call_context */
  voice_srvcc_call_context_type_v02 srvcc_call_context[QMI_VOICE_SRVCC_CALL_CONTEXT_ARRAY_MAX_V02];

  /* Optional */
  /*  Array of Call Alerting Information */
  uint8_t srvcc_alerting_info_valid;  /**< Must be set to true if srvcc_alerting_info is being passed */
  uint32_t srvcc_alerting_info_len;  /**< Must be set to # of elements in srvcc_alerting_info */
  voice_srvcc_alerting_info_type_v02 srvcc_alerting_info[QMI_VOICE_SRVCC_CALL_CONTEXT_ARRAY_MAX_V02];

  /* Optional */
  /*  Number PI Information  */
  uint8_t srvcc_num_pi_info_valid;  /**< Must be set to true if srvcc_num_pi_info is being passed */
  uint32_t srvcc_num_pi_info_len;  /**< Must be set to # of elements in srvcc_num_pi_info */
  voice_srvcc_call_context_num_pi_type_v02 srvcc_num_pi_info[QMI_VOICE_SRVCC_CALL_CONTEXT_ARRAY_MAX_V02];

  /* Optional */
  /*  Array of Caller Name Type Information */
  uint8_t srvcc_caller_name_info_valid;  /**< Must be set to true if srvcc_caller_name_info is being passed */
  uint32_t srvcc_caller_name_info_len;  /**< Must be set to # of elements in srvcc_caller_name_info */
  voice_srvcc_call_context_name_type_v02 srvcc_caller_name_info[QMI_VOICE_SRVCC_CALL_CONTEXT_ARRAY_MAX_V02];
}voice_srvcc_call_config_req_msg_v02;  /* Message */
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_messages
    @{
  */
/** Response Message; Allows third party IMS solutions to provide call information 
	         during SRVCC. */
typedef struct {

  /* Mandatory */
  /*  Result Code */
  qmi_response_type_v01 resp;
}voice_srvcc_call_config_resp_msg_v02;  /* Message */
/**
    @}
  */

/** @addtogroup voice_ims_ext_qmi_messages
    @{
  */
/** Indication Message; Notifies clients about an E911 failure cause code.  */
typedef struct {

  /* Mandatory */
  /*  Call End Reason of E911 Call */
  voice_call_end_reason_type_v02 e911_end_reason;
}voice_e911_orig_fail_ind_msg_v02;  /* Message */
/**
    @}
  */

/* Conditional compilation tags for message removal */ 
//#define REMOVE_QMI_VOICE_E911_ORIG_FAIL_IND_V02 
//#define REMOVE_QMI_VOICE_SRVCC_CALL_CONFIG_V02 

/*Service Message Definition*/
/** @addtogroup voice_ims_ext_qmi_msg_ids
    @{
  */
#define QMI_VOICE_SRVCC_CALL_CONFIG_REQ_V02 0x0064
#define QMI_VOICE_SRVCC_CALL_CONFIG_RESP_V02 0x0064
#define QMI_VOICE_E911_ORIG_FAIL_IND_V02 0x0065
/**
    @}
  */

/* Service Object Accessor */
/** @addtogroup wms_qmi_accessor 
    @{
  */
/** This function is used internally by the autogenerated code.  Clients should use the
   macro voice_ims_ext_get_service_object_v02( ) that takes in no arguments. */
qmi_idl_service_object_type voice_ims_ext_get_service_object_internal_v02
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version );
 
/** This macro should be used to get the service object */ 
#define voice_ims_ext_get_service_object_v02( ) \
          voice_ims_ext_get_service_object_internal_v02( \
            VOICE_IMS_EXT_V02_IDL_MAJOR_VERS, VOICE_IMS_EXT_V02_IDL_MINOR_VERS, \
            VOICE_IMS_EXT_V02_IDL_TOOL_VERS )
/** 
    @} 
  */


#ifdef __cplusplus
}
#endif
#endif

