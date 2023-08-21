/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        S N S _ C L I E N T _ A P I _ V 0 1  . C

GENERAL DESCRIPTION
  This is the file which defines the SNS_CLIENT_SVC service Data structures.

  
  Copyright (c) 2016-2017 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.


  $Header$
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
 *THIS IS AN AUTO GENERATED FILE. DO NOT ALTER IN ANY WAY
 *====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/* This file was generated with Tool version 6.14.7 
   It was generated on: Thu Aug 24 2017 (Spin 0)
   From IDL File: sns_client_api_v01.idl */

#include "stdint.h"
#include "qmi_idl_lib_internal.h"
#include "sns_client_api_v01.h"
#include "common_v01.h"


/*Type Definitions*/
/*Message Definitions*/
static const uint8_t sns_client_req_msg_data_v01[] = {
  0x01,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(sns_client_req_msg_v01, payload),
  ((SNS_CLIENT_REQ_LEN_MAX_V01) & 0xFF), ((SNS_CLIENT_REQ_LEN_MAX_V01) >> 8),
  QMI_IDL_OFFSET8(sns_client_req_msg_v01, payload) - QMI_IDL_OFFSET8(sns_client_req_msg_v01, payload_len),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET16RELATIVE(sns_client_req_msg_v01, use_jumbo_report) - QMI_IDL_OFFSET16RELATIVE(sns_client_req_msg_v01, use_jumbo_report_valid)),
  0x10,
   QMI_IDL_FLAGS_OFFSET_IS_16 | QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET16ARRAY(sns_client_req_msg_v01, use_jumbo_report)
};

static const uint8_t sns_client_resp_msg_data_v01[] = {
  0x02,
   QMI_IDL_AGGREGATE,
  QMI_IDL_OFFSET8(sns_client_resp_msg_v01, resp),
  QMI_IDL_TYPE88(1, 0),

  QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(sns_client_resp_msg_v01, client_id) - QMI_IDL_OFFSET8(sns_client_resp_msg_v01, client_id_valid)),
  0x10,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(sns_client_resp_msg_v01, client_id),

  QMI_IDL_TLV_FLAGS_LAST_TLV | QMI_IDL_TLV_FLAGS_OPTIONAL | (QMI_IDL_OFFSET8(sns_client_resp_msg_v01, result) - QMI_IDL_OFFSET8(sns_client_resp_msg_v01, result_valid)),
  0x11,
   QMI_IDL_GENERIC_4_BYTE,
  QMI_IDL_OFFSET8(sns_client_resp_msg_v01, result)
};

static const uint8_t sns_client_report_ind_msg_data_v01[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(sns_client_report_ind_msg_v01, client_id),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(sns_client_report_ind_msg_v01, payload),
  ((SNS_CLIENT_REPORT_LEN_MAX_V01) & 0xFF), ((SNS_CLIENT_REPORT_LEN_MAX_V01) >> 8),
  QMI_IDL_OFFSET8(sns_client_report_ind_msg_v01, payload) - QMI_IDL_OFFSET8(sns_client_report_ind_msg_v01, payload_len)
};

static const uint8_t sns_client_jumbo_report_ind_msg_data_v01[] = {
  0x01,
   QMI_IDL_GENERIC_8_BYTE,
  QMI_IDL_OFFSET8(sns_client_jumbo_report_ind_msg_v01, client_id),

  QMI_IDL_TLV_FLAGS_LAST_TLV | 0x02,
  QMI_IDL_FLAGS_IS_ARRAY | QMI_IDL_FLAGS_IS_VARIABLE_LEN | QMI_IDL_FLAGS_SZ_IS_16 |   QMI_IDL_GENERIC_1_BYTE,
  QMI_IDL_OFFSET8(sns_client_jumbo_report_ind_msg_v01, payload),
  ((SNS_CLIENT_JUMBO_REPORT_LEN_MAX_V01) & 0xFF), ((SNS_CLIENT_JUMBO_REPORT_LEN_MAX_V01) >> 8),
  QMI_IDL_OFFSET8(sns_client_jumbo_report_ind_msg_v01, payload) - QMI_IDL_OFFSET8(sns_client_jumbo_report_ind_msg_v01, payload_len)
};

/* Type Table */
/* No Types Defined in IDL */

/* Message Table */
static const qmi_idl_message_table_entry SNS_CLIENT_SVC_message_table_v01[] = {
  {sizeof(sns_client_req_msg_v01), sns_client_req_msg_data_v01},
  {sizeof(sns_client_resp_msg_v01), sns_client_resp_msg_data_v01},
  {sizeof(sns_client_report_ind_msg_v01), sns_client_report_ind_msg_data_v01},
  {sizeof(sns_client_jumbo_report_ind_msg_v01), sns_client_jumbo_report_ind_msg_data_v01}
};

/* Range Table */
/* No Ranges Defined in IDL */

/* Predefine the Type Table Object */
static const qmi_idl_type_table_object SNS_CLIENT_SVC_qmi_idl_type_table_object_v01;

/*Referenced Tables Array*/
static const qmi_idl_type_table_object *SNS_CLIENT_SVC_qmi_idl_type_table_object_referenced_tables_v01[] =
{&SNS_CLIENT_SVC_qmi_idl_type_table_object_v01, &common_qmi_idl_type_table_object_v01};

/*Type Table Object*/
static const qmi_idl_type_table_object SNS_CLIENT_SVC_qmi_idl_type_table_object_v01 = {
  0,
  sizeof(SNS_CLIENT_SVC_message_table_v01)/sizeof(qmi_idl_message_table_entry),
  1,
  NULL,
  SNS_CLIENT_SVC_message_table_v01,
  SNS_CLIENT_SVC_qmi_idl_type_table_object_referenced_tables_v01,
  NULL
};

/*Arrays of service_message_table_entries for commands, responses and indications*/
static const qmi_idl_service_message_table_entry SNS_CLIENT_SVC_service_command_messages_v01[] = {
  {QMI_SNS_CLIENT_GET_SUPPORTED_MSGS_REQ_V01, QMI_IDL_TYPE16(1, 0), 0},
  {SNS_CLIENT_REQ_V01, QMI_IDL_TYPE16(0, 0), 1009}
};

static const qmi_idl_service_message_table_entry SNS_CLIENT_SVC_service_response_messages_v01[] = {
  {QMI_SNS_CLIENT_GET_SUPPORTED_MSGS_RESP_V01, QMI_IDL_TYPE16(1, 1), 8204},
  {SNS_CLIENT_RESP_V01, QMI_IDL_TYPE16(0, 1), 25}
};

static const qmi_idl_service_message_table_entry SNS_CLIENT_SVC_service_indication_messages_v01[] = {
  {SNS_CLIENT_REPORT_IND_V01, QMI_IDL_TYPE16(0, 2), 1016},
  {SNS_CLIENT_JUMBO_REPORT_IND_V01, QMI_IDL_TYPE16(0, 3), 62016}
};

/*Service Object*/
struct qmi_idl_service_object SNS_CLIENT_SVC_qmi_idl_service_object_v01 = {
  0x06,
  0x01,
  SNS_CLIENT_SVC_ID_V01,
  62016,
  { sizeof(SNS_CLIENT_SVC_service_command_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(SNS_CLIENT_SVC_service_response_messages_v01)/sizeof(qmi_idl_service_message_table_entry),
    sizeof(SNS_CLIENT_SVC_service_indication_messages_v01)/sizeof(qmi_idl_service_message_table_entry) },
  { SNS_CLIENT_SVC_service_command_messages_v01, SNS_CLIENT_SVC_service_response_messages_v01, SNS_CLIENT_SVC_service_indication_messages_v01},
  &SNS_CLIENT_SVC_qmi_idl_type_table_object_v01,
  0x02,
  NULL
};

/* Service Object Accessor */
qmi_idl_service_object_type SNS_CLIENT_SVC_get_service_object_internal_v01
 ( int32_t idl_maj_version, int32_t idl_min_version, int32_t library_version ){
  if ( SNS_CLIENT_SVC_V01_IDL_MAJOR_VERS != idl_maj_version || SNS_CLIENT_SVC_V01_IDL_MINOR_VERS != idl_min_version
       || SNS_CLIENT_SVC_V01_IDL_TOOL_VERS != library_version)
  {
    return NULL;
  }
  return (qmi_idl_service_object_type)&SNS_CLIENT_SVC_qmi_idl_service_object_v01;
}

