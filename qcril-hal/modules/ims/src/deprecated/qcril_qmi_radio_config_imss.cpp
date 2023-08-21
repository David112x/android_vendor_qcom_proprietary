#ifdef FEATURE_SUPPORT_IMSS_DEPRECATED
/******************************************************************************
#  Copyright (c) 2015-2017, 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/******************************************************************************
  @file    qcril_qmi_radio_config_imss.c
  @brief   qcril qmi - radio config ims handlers

  DESCRIPTION
    Handles imss related radio config req and response handlers

******************************************************************************/

#include "qcril_log.h"
#include "qcril_qmi_client.h"
#include "qcril_qmi_radio_config_imss.h"
#include "qcril_qmi_radio_config_meta.h"
#include "../qcril_qmi_imss.h"
#include "qcril_qmi_imss_qmi.h"
#include "ip_multimedia_subsystem_settings_v01.h"
#include "qcril_reqlist.h"

#if defined(FEATURE_TARGET_GLIBC_x86) || defined(QMI_RIL_UTF)
   extern "C" size_t strlcat(char *, const char *, size_t);
   extern "C" size_t strlcpy(char *, const char *, size_t);
#endif

/*===========================================================================

                               FUNCTIONS

===========================================================================*/

ims_settings_wfc_roaming_enum_v01 qcril_qmi_radio_config_imss_map_radio_config_wifi_roaming_to_ims_wifi_roaming
(
  qcril_qmi_radio_config_imss_voice_over_wifi_roaming_type radio_config_wifi_roaming
)
{
  ims_settings_wfc_roaming_enum_v01 ims_wifi_roaming;
  switch(radio_config_wifi_roaming)
  {
    case QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_ROAMING_DISABLED:
      ims_wifi_roaming = IMS_SETTINGS_WFC_ROAMING_DISABLED_V01;
      break;
    case QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_ROAMING_ENABLED:
      ims_wifi_roaming = IMS_SETTINGS_WFC_ROAMING_ENABLED_V01;
      break;
    default:
      ims_wifi_roaming = IMS_SETTINGS_WFC_ROAMING_NOT_SUPPORTED_V01;
      break;
  }
  QCRIL_LOG_INFO("Mapped radio config wifi roaming: %d, to ims wifi roaming: %d",
                 radio_config_wifi_roaming, ims_wifi_roaming);
  return ims_wifi_roaming;
}

qcril_qmi_radio_config_imss_voice_over_wifi_roaming_type qcril_qmi_radio_config_imss_map_ims_wifi_roaming_to_radio_config_wifi_roaming
(
   ims_settings_wfc_roaming_enum_v01 ims_wifi_roaming
)
{
  qcril_qmi_radio_config_imss_voice_over_wifi_roaming_type radio_config_wifi_roaming;
  switch(ims_wifi_roaming)
  {
    case IMS_SETTINGS_WFC_ROAMING_DISABLED_V01:
      radio_config_wifi_roaming = QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_ROAMING_DISABLED;
      break;
    case IMS_SETTINGS_WFC_ROAMING_ENABLED_V01:
      radio_config_wifi_roaming = QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_ROAMING_ENABLED;
      break;
    default:
      radio_config_wifi_roaming = QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_ROAMING_NONE;
      break;
  }
  QCRIL_LOG_INFO("Mapped ims wifi roaming: %d, to radio config roaming: %d",
                 ims_wifi_roaming, radio_config_wifi_roaming);
  return radio_config_wifi_roaming;
}

ims_settings_wfc_preference_v01 qcril_qmi_radio_config_imss_map_radio_config_wifi_mode_to_ims_wifi_mode
(
  qcril_qmi_radio_config_imss_voice_over_wifi_mode_type radio_config_wifi_mode
)
{
  ims_settings_wfc_preference_v01 ims_wifi_mode;
  switch(radio_config_wifi_mode)
  {
    case QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_WIFI_ONLY:
      ims_wifi_mode = IMS_SETTINGS_WFC_WLAN_ONLY_V01;
      break;
    case QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_CELLULAR_PREFERRED:
      ims_wifi_mode = IMS_SETTINGS_WFC_CELLULAR_PREFERRED_V01;
      break;
    case QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_WIFI_PREFERRED:
      ims_wifi_mode = IMS_SETTINGS_WFC_WLAN_PREFERRED_V01;
      break;
    default:
      ims_wifi_mode = IMS_SETTINGS_WFC_CALL_PREF_NONE_V01;
      break;
  }
  QCRIL_LOG_INFO("Mapped radio config wifi calling preference mode: %d, to ims wifi mode: %d",
                 radio_config_wifi_mode, ims_wifi_mode);
  return ims_wifi_mode;
}

qcril_qmi_radio_config_imss_voice_over_wifi_mode_type qcril_qmi_radio_config_imss_map_ims_wifi_mode_to_radio_config_wifi_mode
(
  ims_settings_wfc_preference_v01 ims_wifi_mode
)
{
  qcril_qmi_radio_config_imss_voice_over_wifi_mode_type radio_config_wifi_mode;
  switch(ims_wifi_mode)
  {
    case IMS_SETTINGS_WFC_WLAN_ONLY_V01:
      radio_config_wifi_mode = QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_WIFI_ONLY;
      break;
    case IMS_SETTINGS_WFC_CELLULAR_PREFERRED_V01:
      radio_config_wifi_mode = QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_CELLULAR_PREFERRED;
      break;
    case IMS_SETTINGS_WFC_WLAN_PREFERRED_V01:
      radio_config_wifi_mode = QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_WIFI_PREFERRED;
      break;
    default:
      radio_config_wifi_mode = QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_NONE;
      break;
  }
  QCRIL_LOG_INFO("Mapped ims wifi calling preference mode: %d, to radio config wifi mode: %d",
                 ims_wifi_mode, radio_config_wifi_mode);
  return radio_config_wifi_mode;
}

/****************************************************************************
*                CONFIG ITEM to QMI MESSAGE MAPPING                         *
*                                                                           *
* 1. QMI_IMSS_GET/SET_VOIP_CONFIG_REQ/RSP_V01                               *
*    - QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_MODE                                 *
*    - QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_WB_MODE                              *
*    - QCRIL_QMI_RADIO_CONFIG_VOIP_SESSION_EXPIRY_TIMER                     *
*    - QCRIL_QMI_RADIO_CONFIG_VOIP_MIN_SESSION_EXPIRY                       *
*    - QCRIL_QMI_RADIO_CONFIG_VOIP_SILENT_REDIAL_ENABLED                    *
*                                                                           *
* 2. QMI_IMSS_GET/SET_SIP_CONFIG_REQ/RSP_V01                                *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_OPERATOR_MODE_A                     *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T1                                  *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T2                                  *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TF                                  *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T4                                  *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TA_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TB_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TD_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TE_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TG_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TH_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TI_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TJ                                  *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TK_VALUE                            *
*    - QCRIL_QMI_RADIO_CONFIG_SIP_KEEPALIVE_ENABLED                         *
*                                                                           *
* 3. QMI_IMSS_GET/SET_REG_MGR_EXTENDED_CONFIG_REQ/RSP_V01                   *
*    - QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_T_DELAY                      *
*    - QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_REG_RETRY_BASE_TIME          *
*    - QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_REG_RETRY_MAX_TIME           *
*                                                                           *
* 4. QMI_IMSS_GET/SET_CLIENT_PROVISIONING_CONFIG_REQ/RSP_V01                *
*    - QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VOLTE              *
*    - QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VT                 *
*    - QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_PRESENCE           *
*    - QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL                 *
*    - QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL_ROAMING         *
*    - QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL_PREFERENCE      *
*    - QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VOWIFI             *
*                                                                           *
* 5. QMI_IMSS_GET/SET_USER_CONFIG_REQ/RSP_V01                               *
*    - QCRIL_QMI_RADIO_CONFIG_USER_IMS_DOMAIN_NAME                          *
*                                                                           *
* 6. QMI_IMSS_GET/SET_SMS_CONFIG_REQ/RSP_V01                                *
*    - QCRIL_QMI_RADIO_CONFIG_SMS_FORMAT                                    *
*    - QCRIL_QMI_RADIO_CONFIG_SMS_OVER_IP                                   *
*    - QCRIL_QMI_RADIO_CONFIG_SMS_PSI                                       *
*                                                                           *
* 7. QMI_IMSS_GET/SET_PRESENCE_CONFIG_REQ/RSP_V01                           *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_PUBLISH_EXPIRY_TIMER                 *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_PUBLISH_EXTENDED_EXPIRY_TIMER        *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITIES_CACHE_EXPIRATION        *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_AVAILABILITY_CACHE_EXPIRATION        *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_POLL_INTERVAL             *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_MINIMUM_PUBLISH_INTERVAL             *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_MAXIMUM_SUBSCRIPTION_LIST_ENTRIES    *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_POLL_                     *
*                                    LIST_SUBSCRIPTION_EXPIRY_TIMER         *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_GZIP_ENABLED                         *
*    - QCRIL_QMI_RADIO_CONFIG_PRESENCE_VOLTE_USER_OPTED_IN_STATUS           *
*                                                                           *
* 8. QMI_IMSS_GET/SET_QIPCALL_CONFIG_REQ/RSP_V01                            *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_MOBILE_DATA_ENABLED                   *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_VOLTE_ENABLED                         *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_VT_CALLING_ENABLED                    *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_SPEECH_START_PORT                     *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_SPEECH_END_PORT                       *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_WB_OCTET_ALIGNED_DYNAMIC_PT       *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_WB_BANDWIDTH_EFFICIENT_DYNAMIC_PT *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_OCTET_ALIGNED_DYNAMIC_PT          *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_BANDWIDTH_EFFICIENT_DYNAMIC_PT    *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_DTMF_WB_DYNAMIC_POINT                 *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_DTMF_NB_DYNAMIC_PT                    *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_DEFAULT_MODE                      *
*                                                                           *
* 9. QMI_IMSS_GET/SET_REG_MGR_CONFIG_REQ/RSP_V01                            *
*    - QCRIL_QMI_RADIO_CONFIG_REG_MGR_CONFIG_REG_MGR_PRIMARY_CSCF           *
*                                                                           *
* 10. QMI_IMSS_GET/SET_HANDOVER_CONFIG_REQ/RSP_V01                          *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD1                *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD2                *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD3                *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_1X_THRESHOLD                  *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WIFI_THRESHOLDA               *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WIFI_THRESHOLDB               *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WLAN_TO_WWAN_HYS_TIMER        *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WLAN_TO_1X_HYS_TIMER          *
*    - QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WWAN_TO_WLAN_HYS_TIMER        *
*                                                                           *
* 11. QMI_IMS_SETTINGS_GET/SET_QIPCALL_VICE_CONFIG_REQ/RSP_V01              *
*    - QCRIL_QMI_RADIO_CONFIG_QIPCALL_VICE_ENABLED                          *
*                                                                           *
*****************************************************************************/

/*===========================================================================
                          GET REQUEST HANDLERS
 Functionality:
 1. Validate the config_params
 2. If valid, get user_data and send QMI async request
    specific to handler.
    And return with SUCCESS in ret_ptr
 3. Else return with FAILURE in ret_ptr
===========================================================================*/

//===========================================================================
// qcril_qmi_radio_config_imss_get_voip_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_voip_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_VOIP_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_voip_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_voip_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void) req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_REG_MGR_EXTENDED_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_reg_mgr_extended_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_client_provisioning_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_client_provisioning_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_CLIENT_PROVISIONING_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_client_provisioning_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_client_provisioning_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_user_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_user_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_USER_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_user_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_user_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_sms_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_sms_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_SMS_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_sms_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_sms_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_presence_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_presence_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_PRESENCE_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_presence_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_presence_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_qipcall_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_QIPCALL_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_qipcall_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_qipcall_config_req_handler


//===========================================================================
// qcril_qmi_radio_config_imss_get_reg_mgr_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_REG_MGR_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_reg_mgr_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_reg_mgr_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_qipcall_vice_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_vice_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  qcril_qmi_ims_config_error_type radio_config_error =
         QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data_len == 0) ||
       (config_ptr->extra_data == NULL))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_GET_QIPCALL_VICE_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_qipcall_vice_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error =
         qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} //qcril_qmi_radio_config_imss_get_qipcall_vice_config_req_handler

/*===========================================================================
                          SET REQUEST HANDLERS
 Functionality:
 1. Validate the config_params and also item value ptr in it
 2. If valid, get user_data and item value
 3. Validate the value type in the config params with the table
    if failure return with specific error
 4. If success, Depending on Config item, store value into the specific
    qmi request structure
 5. send QMI async request with qmi req struct specific to handler.
    And return with SUCCESS in ret_ptr
 3. Else return with FAILURE in ret_ptr
===========================================================================*/

//===========================================================================
// qcril_qmi_radio_config_imss_set_voip_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_voip_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_voip_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  //placeholder for validating integer values
  uint32_t int_value = 0;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_MODE:
        qmi_req.amr_mode_str_valid = TRUE;
        strlcpy(qmi_req.amr_mode_str,
                (const char *)config_ptr->config_item_value,
                IMS_SETTINGS_VOIP_AMR_MODE_STR_LEN_V01);
        QCRIL_LOG_INFO("Set config VOIP amr_mode_str to: %s", qmi_req.amr_mode_str);
        break;

      case QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_WB_MODE:
        qmi_req.amr_wb_mode_str_valid = TRUE;
        strlcpy(qmi_req.amr_wb_mode_str,
                (const char *)config_ptr->config_item_value,
                IMS_SETTINGS_VOIP_AMR_WB_MODE_STR_LEN_V01);
        QCRIL_LOG_INFO("Set config VOIP amr_wb_mode_str to: %s", qmi_req.amr_wb_mode_str);
        break;

      case QCRIL_QMI_RADIO_CONFIG_VOIP_SESSION_EXPIRY_TIMER:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.session_expiry_timer_valid = TRUE;
          qmi_req.session_expiry_timer = int_value;
          QCRIL_LOG_INFO("Set config VOIP session_expiry_timer to: %d", qmi_req.session_expiry_timer);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_VOIP_MIN_SESSION_EXPIRY:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.min_session_expiry_valid = TRUE;
          qmi_req.min_session_expiry = int_value;
          QCRIL_LOG_INFO("Set config VOIP min_session_expiry to: %d", qmi_req.min_session_expiry);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_VOIP_SILENT_REDIAL_ENABLED:
        qmi_req.voip_silent_redial_enabled_valid = TRUE;
        qmi_req.voip_silent_redial_enabled = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config VOIP voip_silent_redial_enabled to: %d", qmi_req.voip_silent_redial_enabled);
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..");
        break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_VOIP_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_voip_config_req_msg_v01),
          sizeof(ims_settings_set_voip_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_voip_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_reg_mgr_extended_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  //placeholder for validating integer values
  uint32_t int_value = 0;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_T_DELAY:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.reregistration_delay_valid = TRUE;
          qmi_req.reregistration_delay = int_value;
          QCRIL_LOG_INFO("Set config REG MGR EXTENDED t_delay to: %d",
                                                   qmi_req.reregistration_delay);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_REG_RETRY_BASE_TIME:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.reg_retry_base_time_valid = TRUE;
          qmi_req.reg_retry_base_time = int_value;
          QCRIL_LOG_INFO("Set config REG MGR EXTENDED reg_retry_base_time_valid to: %d",
                                                   qmi_req.reg_retry_base_time);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_REG_RETRY_MAX_TIME:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.reg_retry_max_time_valid = TRUE;
          qmi_req.reg_retry_max_time = int_value;
          QCRIL_LOG_INFO("Set config REG MGR EXTENDED reg_retry_max_time to: %d",
                                                   qmi_req.reg_retry_max_time);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..")
        break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_REG_MGR_EXTENDED_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_reg_mgr_extended_config_req_msg_v01),
          sizeof(ims_settings_set_reg_mgr_extended_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_client_provisioning_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_client_provisioning_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_client_provisioning_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  //placeholder to validate enum values
  uint32_t enum_int_val;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VOLTE:
        qmi_req.enable_volte_valid = TRUE;
        qmi_req.enable_volte = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config CLIENT PROVISIONING enable_volte to: %d",
                        qmi_req.enable_volte);
        break;

      case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VT:
        qmi_req.enable_vt_valid = TRUE;
        qmi_req.enable_vt = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config CLIENT PROVISIONING enable_vt to: %d",
                        qmi_req.enable_vt);
        break;

      case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VOWIFI:
        qmi_req.enable_vowifi_valid = TRUE;
        qmi_req.enable_vowifi = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config CLIENT PROVISIONING enable_vowifi to: %d",
                        qmi_req.enable_vowifi);
        break;

      case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_PRESENCE:
        qmi_req.enable_presence_valid = TRUE;
        qmi_req.enable_presence = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config CLIENT PROVISIONING enable_presence to: %d", qmi_req.enable_presence);
        break;

      case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL:
        //TODO: misc functions for conversion. and internal enums
        enum_int_val = *((uint32_t *)config_ptr->config_item_value);
        if((enum_int_val == IMS_SETTINGS_WFC_STATUS_NOT_SUPPORTED_V01) ||
           (enum_int_val == IMS_SETTINGS_WFC_STATUS_ON_V01) ||
           (enum_int_val == IMS_SETTINGS_WFC_STATUS_OFF_V01))
        {
          qmi_req.wifi_call_valid = TRUE;
          qmi_req.wifi_call = (ims_settings_wfc_status_enum_v01)enum_int_val;
          QCRIL_LOG_INFO("Set config CLIENT PROVISIONING wifi_call to: %d",
                          qmi_req.wifi_call);
        }
        else
        {
          QCRIL_LOG_INFO("Invalid Wifi call setting..");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL_ROAMING:
        enum_int_val = *((uint32_t *)config_ptr->config_item_value);
        enum_int_val =
            qcril_qmi_radio_config_imss_map_radio_config_wifi_roaming_to_ims_wifi_roaming(
                (qcril_qmi_radio_config_imss_voice_over_wifi_roaming_type) enum_int_val);

        //The Conversion function returns NOT_SUPPORTED
        //if the value is other than Enabled/Disabled,
        //for which error needs to be sent
        if(enum_int_val != IMS_SETTINGS_WFC_ROAMING_NOT_SUPPORTED_V01)
        {
          qmi_req.wifi_call_roaming_valid = TRUE;
          qmi_req.wifi_call_roaming = (ims_settings_wfc_roaming_enum_v01)enum_int_val;
          QCRIL_LOG_INFO("Set config CLIENT PROVISIONING wifi_roaming to: %d",
                          qmi_req.wifi_call_roaming);
        }
        else
        {
          QCRIL_LOG_INFO("Invalid Wifi roaming setting..");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL_PREFERENCE:
        enum_int_val = *((uint32_t *)config_ptr->config_item_value);
        enum_int_val =
            qcril_qmi_radio_config_imss_map_radio_config_wifi_mode_to_ims_wifi_mode(
                (qcril_qmi_radio_config_imss_voice_over_wifi_mode_type)enum_int_val);

        //The Conversion function returns NONE if the value
        //is other than wifi_only/wifi_pref/cellular_pref
        //for which error needs to be sent
        if(enum_int_val != IMS_SETTINGS_WFC_CALL_PREF_NONE_V01)
        {
          qmi_req.wifi_call_preference_valid = TRUE;
          qmi_req.wifi_call_preference = (ims_settings_wfc_preference_v01)enum_int_val;
          QCRIL_LOG_INFO("Set config CLIENT PROVISIONING wifi_call_preference to: %d",
                          qmi_req.wifi_call_preference);
        }
        else
        {
          QCRIL_LOG_INFO("Invalid Wifi preference setting..");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        }
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..");
        break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_CLIENT_PROVISIONING_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_client_provisioning_config_req_msg_v01),
          sizeof(ims_settings_set_client_provisioning_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_client_provisioning_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_user_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_user_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_user_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_USER_IMS_DOMAIN_NAME:
        qmi_req.ims_domain_valid = TRUE;
        strlcpy(qmi_req.ims_domain,
                (const char *)config_ptr->config_item_value,
                IMS_SETTINGS_STRING_LEN_MAX_V01);
        QCRIL_LOG_INFO("Set config USER ims_domain to: %s", qmi_req.ims_domain);
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..")
        break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_USER_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_user_config_req_msg_v01),
          sizeof(ims_settings_set_user_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_user_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_sms_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_sms_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_sms_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  //placeholder to validate enum values
  uint32_t enum_int_val;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_SMS_FORMAT:
        //TODO: misc functions for conversion. and internal enums
        enum_int_val = *((uint32_t *)config_ptr->config_item_value);
        if((enum_int_val == IMS_SETTINGS_SMS_FORMAT_3GPP2_V01) ||
           (enum_int_val == IMS_SETTINGS_SMS_FORMAT_3GPP_V01))
        {
          qmi_req.sms_format_valid = TRUE;
          qmi_req.sms_format = (ims_settings_sms_format_enum_v01)enum_int_val;
          QCRIL_LOG_INFO("Set config SMS sms_format to: %d", qmi_req.sms_format);
        }
        else
        {
          QCRIL_LOG_ERROR("Invalid Sms Format setting..");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_SMS_OVER_IP:
        qmi_req.sms_over_ip_network_indication_valid = TRUE;
        qmi_req.sms_over_ip_network_indication = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SMS sms_over_ip_network_indication to: %d",
                        qmi_req.sms_over_ip_network_indication);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SMS_PSI:
        qmi_req.sms_psi_valid = TRUE;
        strlcpy(qmi_req.sms_psi,
                (const char *)config_ptr->config_item_value,
                IMS_SETTINGS_SMS_PSI_LEN_V01);
        QCRIL_LOG_INFO("Set config SMS sms_psi to: %s", qmi_req.sms_psi);
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..")
        break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_SMS_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_sms_config_req_msg_v01),
          sizeof(ims_settings_set_sms_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_sms_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_presence_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_presence_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_presence_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_PUBLISH_EXPIRY_TIMER:
        qmi_req.publish_expiry_timer_valid = TRUE;
        qmi_req.publish_expiry_timer = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE publish_expiry_timer to: %d",
                        qmi_req.publish_expiry_timer);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_PUBLISH_EXTENDED_EXPIRY_TIMER:
        qmi_req.publish_extended_expiry_timer_valid = TRUE;
        qmi_req.publish_extended_expiry_timer = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE publish_extended_expiry_timer to: %d",
                        qmi_req.publish_extended_expiry_timer);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITIES_CACHE_EXPIRATION:
        qmi_req.capabilites_cache_expiration_valid = TRUE;
        qmi_req.capabilites_cache_expiration = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE capabilites_cache_expiration to: %d",
                        qmi_req.capabilites_cache_expiration);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_AVAILABILITY_CACHE_EXPIRATION:
        qmi_req.availability_cache_expiration_valid = TRUE;
        qmi_req.availability_cache_expiration = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE availability_cache_expiration to: %d",
                        qmi_req.availability_cache_expiration);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_POLL_INTERVAL:
        qmi_req.capability_poll_interval_valid = TRUE;
        qmi_req.capability_poll_interval = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE capability_poll_interval to: %d",
                        qmi_req.capability_poll_interval);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_MINIMUM_PUBLISH_INTERVAL:
        qmi_req.minimum_publish_interval_valid = TRUE;
        qmi_req.minimum_publish_interval = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE minimum_publish_interval to: %d",
                        qmi_req.minimum_publish_interval);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_MAXIMUM_SUBSCRIPTION_LIST_ENTRIES:
        qmi_req.max_subcription_list_entries_valid = TRUE;
        qmi_req.max_subcription_list_entries = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE max_subcription_list_entries to: %d",
                        qmi_req.max_subcription_list_entries);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_POLL_LIST_SUBSCRIPTION_EXPIRY_TIMER:
        qmi_req.capability_poll_list_subscription_expiry_timer_valid = TRUE;
        qmi_req.capability_poll_list_subscription_expiry_timer = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE capability_poll_list_subscription_expiry_timer to: %d",
                        qmi_req.capability_poll_list_subscription_expiry_timer);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_GZIP_ENABLED:
        qmi_req.gzip_enabled_valid = TRUE;
        qmi_req.gzip_enabled = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE gzip_enabled to: %d",
                        qmi_req.gzip_enabled);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_VOLTE_USER_OPTED_IN_STATUS:
        qmi_req.volte_user_opted_in_status_valid = TRUE;
        qmi_req.volte_user_opted_in_status = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE volte_user_opted_in_status to: %d",
                        qmi_req.volte_user_opted_in_status);
        break;

      case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_DISCOVERY_ENABLE:
        qmi_req.capability_discovery_enable_valid = TRUE;
        qmi_req.capability_discovery_enable = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config PRESENCE capability_discovery_enable to: %d",
                        qmi_req.capability_discovery_enable);
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..")
          break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_PRESENCE_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_presence_config_req_msg_v01),
          sizeof(ims_settings_set_presence_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_presence_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_qipcall_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_qipcall_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  //placeholder for validating integer values
  uint32_t int_value = 0;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_MOBILE_DATA_ENABLED:
        qmi_req.mobile_data_enabled_valid = TRUE;
        qmi_req.mobile_data_enabled = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config QIPCALL mobile_data_enabled to: %d",
                        qmi_req.mobile_data_enabled);
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_VOLTE_ENABLED:
        qmi_req.volte_enabled_valid = TRUE;
        qmi_req.volte_enabled = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config QIPCALL volte_enabled to: %d",
                        qmi_req.volte_enabled);
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_VT_CALLING_ENABLED:
        qmi_req.vt_calling_enabled_valid = TRUE;
        qmi_req.vt_calling_enabled = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config QIPCALL vt_calling_enabled to: %d",
                        qmi_req.vt_calling_enabled);
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_SPEECH_START_PORT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.speech_start_port_valid = TRUE;
          qmi_req.speech_start_port = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL speech_start_port to: %d",
                          qmi_req.speech_start_port);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_SPEECH_END_PORT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.speech_end_port_valid = TRUE;
          qmi_req.speech_end_port = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL speech_end_port to: %d",
                          qmi_req.speech_end_port);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_WB_OCTET_ALIGNED_DYNAMIC_PT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.amr_wb_octet_aligned_dynamic_pt_valid = TRUE;
          qmi_req.amr_wb_octet_aligned_dynamic_pt = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL amr_wb_octet_aligned_dynamic_pt to: %d",
                          qmi_req.amr_wb_octet_aligned_dynamic_pt);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_WB_BANDWIDTH_EFFICIENT_DYNAMIC_PT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.amr_wb_bandwidth_efficient_dynamic_pt_valid = TRUE;
          qmi_req.amr_wb_bandwidth_efficient_dynamic_pt = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL amr_wb_bandwidth_efficient_dynamic_pt to: %d",
                          qmi_req.amr_wb_bandwidth_efficient_dynamic_pt);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_OCTET_ALIGNED_DYNAMIC_PT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.amr_octet_aligned_dynamic_pt_valid = TRUE;
          qmi_req.amr_octet_aligned_dynamic_pt = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL amr_octet_aligned_dynamic_pt to: %d",
                          qmi_req.amr_octet_aligned_dynamic_pt);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_BANDWIDTH_EFFICIENT_DYNAMIC_PT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.amr_bandwidth_efficient_dynamic_pt_valid = TRUE;
          qmi_req.amr_bandwidth_efficient_dynamic_pt = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL amr_bandwidth_efficient_dynamic_pt to: %d",
                          qmi_req.amr_bandwidth_efficient_dynamic_pt);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_DTMF_WB_DYNAMIC_POINT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.dtmf_wb_dynamic_pt_valid = TRUE;
          qmi_req.dtmf_wb_dynamic_pt = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL dtmf_wb_dynamic_pt to: %d",
                          qmi_req.dtmf_wb_dynamic_pt);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_DTMF_NB_DYNAMIC_PT:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.dtmf_nb_dynamic_pt_valid = TRUE;
          qmi_req.dtmf_nb_dynamic_pt = int_value;
          QCRIL_LOG_INFO("Set config QIPCALL dtmf_nb_dynamic_pt_valid to: %d",
                          qmi_req.dtmf_nb_dynamic_pt_valid);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_DEFAULT_MODE:
        qmi_req.amr_default_mode_valid = TRUE;
        qmi_req.amr_default_mode = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config QIPCALL amr_default_mode to: %d",
                        qmi_req.amr_default_mode);
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_EMERG_CALL_TIMER:
        qmi_req.emerg_call_timer_valid = TRUE;
        qmi_req.emerg_call_timer = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config QIPCALL emerg_call_timer to: %d",
                        qmi_req.emerg_call_timer);
        break;

      case QCRIL_QMI_RADIO_CONFIG_QIPCALL_VIDEO_QUALITY:
        if (qcril_qmi_radio_config_imss_map_radio_config_video_quality_to_imss_vt_quality(
                (qcril_qmi_radio_config_imss_video_quality)*((uint32_t *)config_ptr->config_item_value),
                &qmi_req.vt_quality_selector))
        {
          qmi_req.vt_quality_selector_valid = TRUE;
          QCRIL_LOG_INFO("Set config QIPCALL vt_quality_selector to: %d",
                          qmi_req.vt_quality_selector);
        }
        else
        {
          QCRIL_LOG_INFO("Invalid parameter received for video quality");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..")
        break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_QIPCALL_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_qipcall_config_req_msg_v01),
          sizeof(ims_settings_set_qipcall_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_qipcall_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_reg_mgr_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_reg_mgr_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_REG_MGR_CONFIG_REG_MGR_PRIMARY_CSCF:
        qmi_req.regmgr_primary_cscf_valid = TRUE;
        strlcpy(qmi_req.regmgr_primary_cscf,
                (const char *)config_ptr->config_item_value,
                IMS_SETTINGS_STRING_LEN_MAX_V01);
        QCRIL_LOG_INFO("Set config REG_MGR regmgr_primary_cscf to: %s",
                        qmi_req.regmgr_primary_cscf);
        break;

      default:
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
        QCRIL_LOG_ERROR("Invalid item..")
        break;
    }
    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_REG_MGR_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_reg_mgr_config_req_msg_v01),
          sizeof(ims_settings_set_reg_mgr_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_reg_mgr_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_qipcall_vice_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_vice_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_qipcall_vice_config_req_msg_v01 qmi_req;
  qcril_qmi_ims_config_item_value_type item_value_type;
  qcril_qmi_ims_config_error_type radio_config_error =
        QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;

  do
  {
    /* Validate the config pointer */
    if( (config_ptr == NULL) ||
        (config_ptr->extra_data_len == 0) ||
        (config_ptr->extra_data == NULL) ||
        (config_ptr->config_item_value_len == 0) ||
        (config_ptr->config_item_value == NULL) )
    {
      QCRIL_LOG_ERROR("Invalid config params");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    memset(&qmi_req, 0, sizeof(qmi_req));

    /* Validate if the item_value_type sent in the config params matches with the
       one in the table. The table should have the right value type for the item */
    item_value_type = qcril_qmi_radio_config_get_item_value_type(config_ptr->config_item);
    if(config_ptr->config_item_value_type != item_value_type)
    {
      QCRIL_LOG_ERROR("Invalid item type..doesnt match with the table");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_ITEM_VALUE_TYPE;
      break;
    }

    QCRIL_LOG_INFO("Config item to set: %s, type: %d",
                   qcril_qmi_radio_config_get_item_log_str(config_ptr->config_item),
                   item_value_type);

    //start with success
    radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;

    if(config_ptr->config_item == QCRIL_QMI_RADIO_CONFIG_QIPCALL_VICE_ENABLED)
    {
      qmi_req.qipcall_vice_enabled_valid = true;
      qmi_req.qipcall_vice_enabled = *((bool *)config_ptr->config_item_value);
      QCRIL_LOG_INFO("Set config QIPCALL qipcall_vice_enabled to: %d",
                      qmi_req.qipcall_vice_enabled);
    }
    else
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
    }

    /* If the qmi req structure generation is SUCCESS then send
       QMI async request */
    if(radio_config_error == QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      qmi_client_error = qmi_client_imss_send_async(
          QMI_IMS_SETTINGS_SET_QIPCALL_VICE_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_qipcall_vice_config_req_msg_v01),
          sizeof(ims_settings_set_qipcall_vice_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error =
         qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}

/*===========================================================================
                           GET RESPONSE HANDLERS
 Functionality:
 1. Check the validity of the config ptr
 2. If not valid, break (eventually sends a Empty response with FAILURE)
 3. Get the request_params_type from the config_ptr
 4. Get the config_item and specific qmi response from request_params_type
 5. Look in the response for SUCCESS
 6. If SUCCESS then look for the specific config item in the response
 7. If present store it in the type field and SUCCESS, else FAILURE.
 8. If item value is obtained successfully then validate the type
    (also put it in config_params to send it for response) and
    call qcril_qmi_imss_get_ims_config_log_and_send_response func.
 9. Else If the response has specific error cause QMI_ERR_CAUSE_CODE_V01
    Then get the mapped radio config error cause and send it in the response
 10. Finally if the error is FAILURE then call the same as above with empty
    payload and FAILURE.
===========================================================================*/

//===========================================================================
// qcril_qmi_radio_config_imss_get_voip_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_voip_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_voip_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for copying the values.
  bool bool_value = 0;
  uint32_t int_value = 0;
  char string_value[QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX];
  memset(&string_value, 0, sizeof(string_value));

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }

    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_voip_config_rsp_msg_v01 *)config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_MODE:
          if(qmi_resp->amr_mode_valid)
          {
            strlcpy(string_value,
                    (const char *)qcril_qmi_radio_config_get_string_amr_or_amr_wb_mode(qmi_resp->amr_mode,QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_MODE_MAX_INDEX),
                    QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX);
            config_params.config_item_value = (void*)&string_value;
            config_params.config_item_value_len = strlen(string_value);
            QCRIL_LOG_INFO("get VOIP amr_mode value: %d, converted to: %s",
                            qmi_resp->amr_mode, string_value);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_WB_MODE:
          if(qmi_resp->amr_wb_mode_valid)
          {
            strlcpy(string_value,
                    (const char *)qcril_qmi_radio_config_get_string_amr_or_amr_wb_mode(qmi_resp->amr_wb_mode,QCRIL_QMI_RADIO_CONFIG_VOIP_AMR_WB_MODE_MAX_INDEX),
                    QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX);
            config_params.config_item_value = (void*)&string_value;
            config_params.config_item_value_len = strlen(string_value);
            QCRIL_LOG_INFO("get VOIP amr_wb_mode value: %d, converted to: %s",
                            qmi_resp->amr_wb_mode, string_value);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_VOIP_SESSION_EXPIRY_TIMER:
          if(qmi_resp->session_expiry_timer_valid)
          {
            int_value = qmi_resp->session_expiry_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get VOIP session_expiry_timer value: %d",
                            qmi_resp->session_expiry_timer);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_VOIP_MIN_SESSION_EXPIRY:
          if(qmi_resp->min_session_expiry_valid)
          {
            int_value = qmi_resp->min_session_expiry;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get VOIP min_session_expiry value: %d",
                            qmi_resp->min_session_expiry);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_VOIP_SILENT_REDIAL_ENABLED:
          if(qmi_resp->voip_silent_redial_enabled_valid)
          {
            bool_value = (qmi_resp->voip_silent_redial_enabled) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get VOIP voip_silent_redial_enabled value: %d",
                            qmi_resp->voip_silent_redial_enabled);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, radio_config_error is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_voip_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_reg_mgr_extended_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for copying the values.
  uint32_t int_value = 0;
  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_reg_mgr_extended_config_rsp_msg_v01 *) config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }


    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_T_DELAY:
          if(qmi_resp->reregistration_delay_valid)
          {
            int_value = qmi_resp->reregistration_delay;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get REG MGR EXT t_delay value: %d",
                            qmi_resp->reregistration_delay);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_REG_RETRY_BASE_TIME:
          if(qmi_resp->reg_retry_base_time_valid)
          {
            int_value = qmi_resp->reg_retry_base_time;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get REG MGR EXT reg_retry_base_time value: %d",
                            qmi_resp->reg_retry_base_time);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_REG_MGR_EXTENDED_REG_RETRY_MAX_TIME:
          if(qmi_resp->reg_retry_max_time_valid)
          {
            int_value = qmi_resp->reg_retry_max_time;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get REG MGR EXT reg_retry_max_time value: %d",
                            qmi_resp->reg_retry_max_time);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the settings resp is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present )
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_client_provisioning_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_client_provisioning_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_client_provisioning_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for copying values.
  bool bool_value = 0;
  int int_value = 0;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_client_provisioning_config_rsp_msg_v01 *)config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VOLTE:
          if(qmi_resp->enable_volte_valid)
          {
            bool_value = (qmi_resp->enable_volte) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get CLIENT PROVISIONING enable_volte value: %d",
                            qmi_resp->enable_volte);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VOWIFI:
          if(qmi_resp->enable_vowifi_valid)
          {
            bool_value = (qmi_resp->enable_vowifi) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get CLIENT PROVISIONING enable_vowifi value: %d",
                            qmi_resp->enable_vowifi);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_VT:
          if(qmi_resp->enable_vt_valid)
          {
            bool_value = (qmi_resp->enable_vt) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get CLIENT PROVISIONING enable_vt value: %d",
                            qmi_resp->enable_vt);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_ENABLE_PRESENCE:
          if(qmi_resp->enable_presence_valid)
          {
            bool_value = (qmi_resp->enable_presence) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get CLIENT PROVISIONING enable_presence value: %d",
                            qmi_resp->enable_presence);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL:
          //TODO: misc functions for conversion. and internal enums
          if( (qmi_resp->wifi_call_valid) &&
              (qmi_resp->wifi_call == IMS_SETTINGS_WFC_STATUS_NOT_SUPPORTED_V01 ||
               qmi_resp->wifi_call == IMS_SETTINGS_WFC_STATUS_ON_V01 ||
               qmi_resp->wifi_call == IMS_SETTINGS_WFC_STATUS_OFF_V01) )
          {
            int_value = qmi_resp->wifi_call;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get CLIENT PROVISIONING wifi_call value: %d",
                            qmi_resp->wifi_call);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL_ROAMING:
          if( (qmi_resp->wifi_call_roaming_valid) )
          {
            int_value =
                qcril_qmi_radio_config_imss_map_ims_wifi_roaming_to_radio_config_wifi_roaming(qmi_resp->wifi_call_roaming);

            //None is sent by the conversion function if the value is one
            //of the values which are not supported by framework.
            //Supported ones: enable/disable
            if(int_value != QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_ROAMING_NONE)
            {
              config_params.config_item_value_len = sizeof(int_value);
              config_params.config_item_value = (void*) &int_value;
              QCRIL_LOG_INFO("get CLIENT PROVISIONING wifi_call_roaming value: %d",
                              int_value);
            }
            else
            {
              QCRIL_LOG_INFO("Value %d not supported, hence send error", qmi_resp->wifi_call_roaming);
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
            }
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_CLIENT_PROVISIONING_WIFI_CALL_PREFERENCE:
          //TODO: misc functions for conversion. and internal enums
          if( (qmi_resp->wifi_call_preference_valid) )
          {
            int_value =
                qcril_qmi_radio_config_imss_map_ims_wifi_mode_to_radio_config_wifi_mode(qmi_resp->wifi_call_preference);

            //None is sent by the conversion function if the value is one
            //of the values which are not supported by framework.
            //Supported ones: wifi_only, wifi_pref, cellular_pref
            if(int_value != QCRIL_QMI_RADIO_CONFIG_IMSS_VOICE_OVER_WIFI_MODE_NONE)
            {
              config_params.config_item_value_len = sizeof(int_value);
              config_params.config_item_value = (void*) &int_value;
              QCRIL_LOG_INFO("get CLIENT PROVISIONING wifi_call_preference value: %d",
                              int_value);
            }
            else
            {
              QCRIL_LOG_INFO("Value %d not supported, hence send error", qmi_resp->wifi_call_preference);
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
            }
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_client_provisioning_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_user_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_user_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_user_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //placeholder to copy string value, to avoid malloc
  char string_value[QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX];
  memset(&string_value, 0, sizeof(string_value));

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_user_config_rsp_msg_v01 *) config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }


    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_USER_IMS_DOMAIN_NAME:
          if(qmi_resp->ims_domain_valid)
          {
            config_params.config_item_value_len = strlen(qmi_resp->ims_domain);
            strlcpy(string_value,
                    (const char *)qmi_resp->ims_domain,
                    IMS_SETTINGS_STRING_LEN_MAX_V01);
            config_params.config_item_value = (void*)&string_value;
            QCRIL_LOG_INFO("get USER ims_domain value: %s", string_value);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_user_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_sms_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_sms_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_sms_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for copying values.
  bool bool_value = 0;
  uint32_t int_value = 0;

  //placeholder to copy string value, to avoid malloc
  char string_value[QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX];
  memset(&string_value, 0, sizeof(string_value));

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_sms_config_rsp_msg_v01 *)config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }


    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_SMS_FORMAT:
          //TODO: misc functions for conversion. and internal enums
          if( (qmi_resp->sms_format_valid) &&
              (qmi_resp->sms_format == IMS_SETTINGS_SMS_FORMAT_3GPP2_V01 ||
               qmi_resp->sms_format == IMS_SETTINGS_SMS_FORMAT_3GPP_V01) )
          {
            int_value = qmi_resp->sms_format;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SMS sms_format value: %d",
                            qmi_resp->sms_format);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SMS_OVER_IP:
          if(qmi_resp->sms_over_ip_network_indication_valid)
          {
            bool_value = (qmi_resp->sms_over_ip_network_indication) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get SMS sms_over_ip_network_indication value: %d",
                            qmi_resp->sms_over_ip_network_indication);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SMS_PSI:
          if(qmi_resp->sms_psi_valid)
          {
            config_params.config_item_value_len = strlen(qmi_resp->sms_psi);
            strlcpy(string_value,
                    (const char *)qmi_resp->sms_psi,
                    IMS_SETTINGS_SMS_PSI_LEN_V01);
            config_params.config_item_value = (void*)&string_value;
            QCRIL_LOG_INFO("get SMS sms_psi value: %s", string_value);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_sms_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_presence_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_presence_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_presence_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for copying values.
  bool bool_value = 0;
  uint32_t int_value = 0;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_presence_config_rsp_msg_v01 *) config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_PUBLISH_EXPIRY_TIMER:
          if(qmi_resp->publish_expiry_timer_valid)
          {
            int_value = qmi_resp->publish_expiry_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE publish_expiry_timer value: %d",
                            qmi_resp->publish_expiry_timer);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_PUBLISH_EXTENDED_EXPIRY_TIMER:
          if(qmi_resp->publish_extended_expiry_timer_valid)
          {
            int_value = qmi_resp->publish_extended_expiry_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE publish_extended_expiry_timer value: %d",
                            qmi_resp->publish_extended_expiry_timer);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITIES_CACHE_EXPIRATION:
          if(qmi_resp->capabilites_cache_expiration_valid)
          {
            int_value = qmi_resp->capabilites_cache_expiration;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE capabilites_cache_expiration value: %d",
                            qmi_resp->capabilites_cache_expiration);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_AVAILABILITY_CACHE_EXPIRATION:
          if(qmi_resp->availability_cache_expiration_valid)
          {
            int_value = qmi_resp->availability_cache_expiration;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE availability_cache_expiration value: %d",
                            qmi_resp->availability_cache_expiration);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_POLL_INTERVAL:
          if(qmi_resp->capability_poll_interval_valid)
          {
            int_value = qmi_resp->capability_poll_interval;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE capability_poll_interval value: %d",
                            qmi_resp->capability_poll_interval);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_MINIMUM_PUBLISH_INTERVAL:
          if(qmi_resp->minimum_publish_interval_valid)
          {
            int_value = qmi_resp->minimum_publish_interval;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE minimum_publish_interval value: %d",
                            qmi_resp->minimum_publish_interval);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_MAXIMUM_SUBSCRIPTION_LIST_ENTRIES:
          if(qmi_resp->max_subcription_list_entries_valid)
          {
            int_value = qmi_resp->max_subcription_list_entries;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE max_subcription_list_entries value: %d",
                            qmi_resp->max_subcription_list_entries);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_POLL_LIST_SUBSCRIPTION_EXPIRY_TIMER:
          if(qmi_resp->capability_poll_list_subscription_expiry_timer_valid)
          {
            int_value = qmi_resp->capability_poll_list_subscription_expiry_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get PRESENCE capability_poll_list_subscription_expiry_timer value: %d",
                            qmi_resp->capability_poll_list_subscription_expiry_timer);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_GZIP_ENABLED:
          if(qmi_resp->gzip_enabled_valid)
          {
            bool_value = (qmi_resp->gzip_enabled) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get PRESENCE gzip_enabled value: %d",
                            qmi_resp->gzip_enabled);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_VOLTE_USER_OPTED_IN_STATUS:
          if(qmi_resp->volte_user_opted_in_status_valid)
          {
            bool_value = (qmi_resp->volte_user_opted_in_status) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get PRESENCE volte_user_opted_in_status value: %d",
                            qmi_resp->volte_user_opted_in_status);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_PRESENCE_CAPABILITY_DISCOVERY_ENABLE:
          if(qmi_resp->capability_discovery_enable_valid)
          {
            bool_value = (qmi_resp->capability_discovery_enable) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get PRESENCE capability_discovery_enable value: %d",
                            qmi_resp->capability_discovery_enable);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_presence_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_qipcall_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_qipcall_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for boolean value
  bool bool_value;
  uint32_t int_value = 0;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_qipcall_config_rsp_msg_v01 *) config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_MOBILE_DATA_ENABLED:
          if(qmi_resp->mobile_data_enabled_valid)
          {
            bool_value = (qmi_resp->mobile_data_enabled) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get QIPCALL mobile_data_enabled value: %d",
                            qmi_resp->mobile_data_enabled);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_VOLTE_ENABLED:
          if(qmi_resp->volte_enabled_valid)
          {
            bool_value = (qmi_resp->volte_enabled) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get QIPCALL volte_enabled value: %d",
                            qmi_resp->volte_enabled);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_VT_CALLING_ENABLED:
          if(qmi_resp->vt_calling_enabled_valid)
          {
            bool_value = (qmi_resp->vt_calling_enabled) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get QIPCALL vt_calling_enabled value: %d",
                            qmi_resp->vt_calling_enabled);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_SPEECH_START_PORT:
          if(qmi_resp->speech_start_port_valid)
          {
            int_value = qmi_resp->speech_start_port;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL speech_start_port value: %d",
                            qmi_resp->speech_start_port);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_SPEECH_END_PORT:
          if(qmi_resp->speech_end_port_valid)
          {
            int_value = qmi_resp->speech_end_port;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL speech_end_port value: %d",
                            qmi_resp->speech_end_port);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_WB_OCTET_ALIGNED_DYNAMIC_PT:
          if(qmi_resp->amr_wb_octet_aligned_dynamic_pt_valid)
          {
            int_value = qmi_resp->amr_wb_octet_aligned_dynamic_pt;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL amr_wb_octet_aligned_dynamic_pt value: %d",
                            qmi_resp->amr_wb_octet_aligned_dynamic_pt);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_WB_BANDWIDTH_EFFICIENT_DYNAMIC_PT:
          if(qmi_resp->amr_wb_bandwidth_efficient_dynamic_pt_valid)
          {
            int_value = qmi_resp->amr_wb_bandwidth_efficient_dynamic_pt;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL amr_wb_bandwidth_efficient_dynamic_pt: %d",
                            qmi_resp->amr_wb_bandwidth_efficient_dynamic_pt);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_OCTET_ALIGNED_DYNAMIC_PT:
          if(qmi_resp->amr_octet_aligned_dynamic_pt_valid)
          {
            int_value = qmi_resp->amr_octet_aligned_dynamic_pt;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL amr_octet_aligned_dynamic_pt value: %d",
                            qmi_resp->amr_octet_aligned_dynamic_pt);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_BANDWIDTH_EFFICIENT_DYNAMIC_PT:
          if(qmi_resp->amr_bandwidth_efficient_dynamic_pt_valid)
          {
            int_value = qmi_resp->amr_bandwidth_efficient_dynamic_pt;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL amr_bandwidth_efficient_dynamic_pt value: %d",
                            qmi_resp->amr_bandwidth_efficient_dynamic_pt);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_DTMF_WB_DYNAMIC_POINT:
          if(qmi_resp->dtmf_wb_dynamic_pt_valid)
          {
            int_value = qmi_resp->dtmf_wb_dynamic_pt;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL dtmf_wb_dynamic_pt value: %d",
                            qmi_resp->dtmf_wb_dynamic_pt);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_DTMF_NB_DYNAMIC_PT:
          if(qmi_resp->dtmf_nb_dynamic_pt_valid)
          {
            int_value = qmi_resp->dtmf_nb_dynamic_pt;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL dtmf_nb_dynamic_pt value: %d",
                            qmi_resp->dtmf_nb_dynamic_pt);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_AMR_DEFAULT_MODE:
          if(qmi_resp->amr_default_mode_valid)
          {
            bool_value = (qmi_resp->amr_default_mode) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get QIPCALL amr_default_mode value: %d",
                            qmi_resp->amr_default_mode);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_EMERG_CALL_TIMER:
          if(qmi_resp->emerg_call_timer_valid)
          {
            int_value = qmi_resp->emerg_call_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get QIPCALL emerg_call_timer value: %d",
                            qmi_resp->emerg_call_timer);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_QIPCALL_VIDEO_QUALITY:
          if(qmi_resp->vt_quality_selector_valid)
          {
            if (qcril_qmi_radio_config_imss_map_imss_vt_quality_to_radio_config_video_quality(
                    qmi_resp->vt_quality_selector, (qcril_qmi_radio_config_imss_video_quality *)&int_value))
            {
              config_params.config_item_value_len = sizeof(int_value);
              config_params.config_item_value = (void*) &int_value;
              QCRIL_LOG_INFO("get QIPCALL vt_quality_selector value: %d",
                              qmi_resp->vt_quality_selector);
            }
            else
            {
              QCRIL_LOG_INFO("Invalid parameter recevied in vt_quality_selector = %d",
                      qmi_resp->vt_quality_selector);
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
            }
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_qipcall_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_reg_mgr_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_reg_mgr_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //placeholder to copy string value, to avoid malloc
  char string_value[QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX];
  memset(&string_value, 0, sizeof(string_value));

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_reg_mgr_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }


    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      switch(config_ptr->config_item)
      {
        case QCRIL_QMI_RADIO_CONFIG_REG_MGR_CONFIG_REG_MGR_PRIMARY_CSCF:
          if(qmi_resp->regmgr_primary_cscf_valid)
          {
            config_params.config_item_value_len = strlen(qmi_resp->regmgr_primary_cscf);
            strlcpy(string_value,
                    (const char *)qmi_resp->regmgr_primary_cscf,
                    IMS_SETTINGS_STRING_LEN_MAX_V01);
            config_params.config_item_value = (void*)&string_value;
            QCRIL_LOG_INFO("get REG_MGR regmgr_primary_cscf value: %s", string_value);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        default:
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
          QCRIL_LOG_INFO("Item not valid in qmi structure..");
          break;
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_get_reg_mgr_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_qipcall_vice_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_vice_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_qipcall_vice_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error =
        QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for copying values.
  bool bool_value = 0;

  do
  {
   /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;
    config_params.config_item_value_type =
        qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_qipcall_vice_config_rsp_msg_v01 *) config_ptr->extra_data;
    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then process the data */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      //Check for the item requested by the request and initialize it to the value to be sent back
      if(config_ptr->config_item == QCRIL_QMI_RADIO_CONFIG_QIPCALL_VICE_ENABLED)
      {
        if(qmi_resp->qipcall_vice_enabled_valid)
        {
          bool_value = (qmi_resp->qipcall_vice_enabled) ? TRUE : FALSE;
          config_params.config_item_value_len = sizeof(bool_value);
          config_params.config_item_value = (void*) &bool_value;
          QCRIL_LOG_INFO("get HANDOVER qipcall_vice_enabled_valid value: %d",
                          qmi_resp->qipcall_vice_enabled);
        }
        else
        {
          QCRIL_LOG_INFO("Did not get the value requested in qmi message");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
        }
      }
      if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
      {
        break;
      }
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error =
        qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above, radio_config_error is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error =
        qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }
    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
        &config_params,
        radio_config_error,
        ((error_cause_code_present) ?
        qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(
            qmi_resp->settings_resp) :
        QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if((radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS) && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_get_ims_config_log_and_send_response( req_id,
                                                     NULL,
                                                     radio_config_error,
                                                     QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}//qcril_qmi_radio_config_imss_get_qipcall_vice_config_resp_handler

/*===========================================================================
                           SET RESPONSE HANDLERS
  Functionality:
 1. Check the validity of the config ptr
 2. If not valid, break (eventually sends a Empty response with FAILURE)
 3. Get the request_params_type from the config_ptr
 4. Get the config_item and specific qmi response from request_params_type
 5. If the response is success, send the response with config item
 6. Else If the response has specific error cause QMI_ERR_CAUSE_CODE_V01
    Then get the mapped radio config error cause and send it in the response
 7. Finally if the error is FAILURE then call the same as above with empty
    payload and FAILURE.
===========================================================================*/

//===========================================================================
// qcril_qmi_radio_config_imss_set_voip_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_voip_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_voip_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_voip_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_voip_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_reg_mgr_extended_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_reg_mgr_extended_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present )
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_client_provisioning_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_client_provisioning_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_client_provisioning_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_client_provisioning_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }


    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_client_provisioning_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_user_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_user_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_user_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_user_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_user_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_sms_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_sms_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_sms_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_sms_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_sms_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_presence_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_presence_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_presence_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_presence_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is present
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_presence_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_qipcall_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_qipcall_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_qipcall_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_qipcall_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_reg_mgr_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();


  ims_settings_set_reg_mgr_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_reg_mgr_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         &config_params,
                                                         radio_config_error,
                                                         ((error_cause_code_present) ?
                                                         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(qmi_resp->settings_resp) :
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                         NULL,
                                                         radio_config_error,
                                                         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_reg_mgr_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_qipcall_vice_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_vice_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_qipcall_vice_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error =
      QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  do
  {
    /* Validate config ptr */
    if((config_ptr == NULL) ||
       (config_ptr->extra_data == NULL) ||
       (config_ptr->extra_data_len <=0))
    {
      QCRIL_LOG_ERROR("Invalid config params..");
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_CONFIG_PARAMS;
      break;
    }
    /* Create a basic config_params structure
       which will be used to send the response*/
    config_params.config_item = config_ptr->config_item;

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_set_qipcall_vice_config_rsp_msg_v01 *) config_ptr->extra_data;

    if(qmi_resp == NULL)
    {
      QCRIL_LOG_ERROR("Params data is null..");
      break;
    }

    /* If the qmi response is success, then send success */
    if(qmi_resp->resp.result == QMI_RESULT_SUCCESS_V01)
    {
      radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
      QCRIL_LOG_INFO("response success");
    }
    /* otherwise, If the response is specifically ERR_CAUSE_CODE
     * specifically look if settings response is sent by the modem
     * if it is then send the settings resp else empty response
     * is sent */
    else if(qmi_resp->resp.error == QMI_ERR_CAUSE_CODE_V01)
    {
      radio_config_error =
          qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      if(qmi_resp->settings_resp_valid)
      {
        //flag to determine if the FailureCause is sent
        error_cause_code_present = TRUE;
      }
      else
      {
        QCRIL_LOG_INFO("qmi error response");
        break;
      }
    }
    /* If not any of the above,
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error =
          qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(qmi_resp->resp.error);
      break;
    }

    /* Send response with success or error cause and config item in params */
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
         &config_params,
         radio_config_error,
         ((error_cause_code_present) ?
         qcril_qmi_radio_config_map_qmi_settings_resp_to_radio_config_settings_resp(
             qmi_resp->settings_resp) :
         QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR) );
  }while(FALSE);

  /* If ril_err is GENERIC_FAILURE and also if error cause code response
   * is not sent, send an empty payload response back */
  if((radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS) && !error_cause_code_present)
  {
    QCRIL_LOG_INFO("Sending Error response..");
    qcril_qmi_imss_set_ims_config_log_and_send_response( req_id,
                                                     NULL,
                                                     radio_config_error,
                                                     QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
  }
  QCRIL_LOG_FUNC_RETURN_WITH_RET(0);
  return QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
}// qcril_qmi_radio_config_imss_set_qipcall_vice_config_resp_handler
#endif
