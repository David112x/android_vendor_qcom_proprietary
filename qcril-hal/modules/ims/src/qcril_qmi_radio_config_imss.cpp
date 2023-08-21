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
#include "qcril_qmi_imss.h"
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

//Helper functions for voip string conversion.
//TODO: next phase, to be moved to misc.c file
static char qcril_qmi_radio_config_string_value[QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX];

/* According to spec RFC 4867 the value for amr/amrwb mode is comma
 * seperated list of modes amr(0-7) amrwb(0-8) e.g., "1,4,6".
 *
 * Values and mapping for AMR mode:
 *   - 0x1  -- 0 (4.75 kbps)
 *   - 0x2  -- 1 (5.15 kbps)
 *   - 0x4  -- 2 (5.9 kbps)
 *   - 0x8  -- 3 (6.17 kbps)
 *   - 0x10 -- 4 (7.4 kbps)
 *   - 0x20 -- 5 (7.95 kbps)
 *   - 0x40 -- 6 (10.2 kbps)
 *   - 0x80 -- 7 (12.2 kbps)
 **
 * Values and mapping foro AMR WB mode:
 *   - 0x1  -- 0 (6.60 kbps)
 *   - 0x2  -- 1 (8.85 kbps)
 *   - 0x4  -- 2 (12.65 kbps)
 *   - 0x8  -- 3 (14.25 kbps)
 *   - 0x10 -- 4 (15.85 kbps)
 *   - 0x20 -- 5 (18.25 kbps)
 *   - 0x40 -- 6 (19.85 kbps)
 *   - 0x80 -- 7 (23.05 kbps)
 *   - 0x100 -- 8 (23.85 kbps)
 *
 *   The Below function qcril_qmi_radio_config_get_string_amr_or_amr_wb_mode
 *   converts the byte value e.g.0x5 (0x1|0x4) to string e.g. "0,2"
 */
char* qcril_qmi_radio_config_get_string_amr_or_amr_wb_mode(uint16_t byte_value, uint32_t max_index)
{
  memset(qcril_qmi_radio_config_string_value, 0, sizeof(qcril_qmi_radio_config_string_value));

  uint32_t i = 0;
  bool is_first = TRUE;
  QCRIL_LOG_INFO("byte value: %x", byte_value);
  for(i = 0; i < max_index; i++)
  {
    //check if it is value corresponding to i
    if( (byte_value & (0x01 << i)) != 0 )
    {
      if(is_first)
      {
        //add the value without comma
        snprintf(qcril_qmi_radio_config_string_value,
                 QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX,
                 "%d", i);
        is_first = FALSE;
      }
      else
      {
        snprintf(qcril_qmi_radio_config_string_value + strlen(qcril_qmi_radio_config_string_value),
                 QCRIL_QMI_RADIO_CONFIG_SETTINGS_STRING_LEN_MAX,
                 ",%d", i);
      }
    }
  }
  QCRIL_LOG_INFO("Converted string: %s", qcril_qmi_radio_config_string_value);
  return qcril_qmi_radio_config_string_value;
}

/* Refer above comment for the formatting of string value and the mode mask */
uint64_t qcril_qmi_radio_config_get_amr_or_amr_wb_mode_mask(const char *val, long mode_max)
{
  const char delim[2] = {',',0};
  char *token, *end_ptr, *saveptr;
  uint64_t mask = 0;
  long mode = 0;

  strlcpy(qcril_qmi_radio_config_string_value, val, sizeof(qcril_qmi_radio_config_string_value));
  QCRIL_LOG_INFO("mask string: %s", qcril_qmi_radio_config_string_value);
  token = strtok_r(qcril_qmi_radio_config_string_value, delim, &saveptr);
  while (token)
  {
    mode = strtol(token, &end_ptr, 0);
    if ((errno != EINVAL) && (errno != ERANGE) && (end_ptr != token) &&
        (mode >= 0) && (mode <= mode_max))
    {
      mask = mask | (0x01 << mode);
    }
    token = strtok_r(NULL, delim, &saveptr);
  }

  QCRIL_LOG_INFO("Converted to mask 0x%x", mask);
  return mask;
}

//===========================================================================
// qcril_qmi_radio_config_imss_get_sip_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_sip_config_req_handler
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
        QMI_IMS_SETTINGS_GET_SIP_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_sip_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb_v02,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int) qmi_client_error);
    radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_sip_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_handover_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_handover_config_req_handler
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
        QMI_IMS_SETTINGS_GET_HANDOVER_CONFIG_REQ_V01,
        NULL,
        0,
        sizeof(ims_settings_get_handover_config_rsp_msg_v01),
        qcril_qmi_imss_config_resp_cb_v02,
        config_ptr->extra_data);

    QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
    radio_config_error =
       qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
} // qcril_qmi_radio_config_imss_get_handover_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_sip_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_sip_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_sip_config_req_msg_v01 qmi_req;
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
      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_OPERATOR_MODE_A:
        qmi_req.sip_timer_operator_mode_a_valid = TRUE;
        qmi_req.sip_timer_operator_mode_a = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP sip_timer_operator_mode_a to: %d",
                        qmi_req.sip_timer_operator_mode_a);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T1:
        qmi_req.timer_t1_valid = TRUE;
        qmi_req.timer_t1 = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_t1 to: %d", qmi_req.timer_t1);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T2:
        qmi_req.timer_t2_valid = TRUE;
        qmi_req.timer_t2 = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_t2 to: %d", qmi_req.timer_t2);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TF:
        qmi_req.timer_tf_valid = TRUE;
        qmi_req.timer_tf = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_tf to: %d", qmi_req.timer_tf);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T4:
        qmi_req.timer_t4_valid = TRUE;
        qmi_req.timer_t4 = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_t4 to: %d", qmi_req.timer_t4);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TA_VALUE:
        qmi_req.timer_ta_value_valid = TRUE;
        qmi_req.timer_ta_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_ta_value to: %d", qmi_req.timer_ta_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TB_VALUE:
        qmi_req.timer_tb_value_valid = TRUE;
        qmi_req.timer_tb_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_tb_value to: %d", qmi_req.timer_tb_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TD_VALUE:
        qmi_req.timer_td_value_valid = TRUE;
        qmi_req.timer_td_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_td_value to: %d", qmi_req.timer_td_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TE_VALUE:
        qmi_req.timer_te_value_valid = TRUE;
        qmi_req.timer_te_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_te_value to: %d", qmi_req.timer_te_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TG_VALUE:
        qmi_req.timer_tg_value_valid = TRUE;
        qmi_req.timer_tg_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_tg_value to: %d", qmi_req.timer_tg_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TH_VALUE:
        qmi_req.timer_th_value_valid = TRUE;
        qmi_req.timer_th_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_th_value to: %d", qmi_req.timer_th_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TI_VALUE:
        qmi_req.timer_ti_value_valid = TRUE;
        qmi_req.timer_ti_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_ti_value to: %d", qmi_req.timer_ti_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TJ:
        int_value = *((uint32_t *)config_ptr->config_item_value);
        if(int_value <= UINT16_MAX)
        {
          qmi_req.timer_tj_valid = TRUE;
          qmi_req.timer_tj = int_value;
          QCRIL_LOG_INFO("Set config SIP timer_tj to: %d", qmi_req.timer_tj);
        }
        else
        {
          QCRIL_LOG_INFO("Value is greater than the limit!");
          radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
        }
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TK_VALUE:
        qmi_req.timer_tk_value_valid = TRUE;
        qmi_req.timer_tk_value = *((uint32_t *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP timer_tk_value to: %d", qmi_req.timer_tk_value);
        break;

      case QCRIL_QMI_RADIO_CONFIG_SIP_KEEPALIVE_ENABLED:
        qmi_req.keepalive_enabled_valid = TRUE;
        qmi_req.keepalive_enabled = *((bool *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config SIP keepalive_enabled to: %d", qmi_req.keepalive_enabled);
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
          QMI_IMS_SETTINGS_SET_SIP_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_sip_config_req_msg_v01),
          sizeof(ims_settings_set_sip_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb_v02,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error = qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}// qcril_qmi_radio_config_imss_set_sip_config_req_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_handover_config_req_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_handover_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();
  (void)req_id;
  ims_settings_set_handover_config_req_msg_v01 qmi_req;
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

    /* Depending on the Config item,
       validate the type processed and put it in the request*/
    switch(config_ptr->config_item)
    {
      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD1:
        qmi_req.iIMSHOLTEQualTH1_valid = TRUE;
        qmi_req.iIMSHOLTEQualTH1 = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER iIMSHOLTEQualTH1 to: %d",
                        qmi_req.iIMSHOLTEQualTH1);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD2:
        qmi_req.iIMSHOLTEQualTH2_valid = TRUE;
        qmi_req.iIMSHOLTEQualTH2 = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER iIMSHOLTEQualTH2 to: %d",
                        qmi_req.iIMSHOLTEQualTH2);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD3:
        qmi_req.iIMSHOLTEQualTH3_valid = TRUE;
        qmi_req.iIMSHOLTEQualTH3 = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER iIMSHOLTEQualTH3 to: %d",
                        qmi_req.iIMSHOLTEQualTH3);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_1X_THRESHOLD:
        qmi_req.iIMSHO1xQualTH_valid = TRUE;
        qmi_req.iIMSHO1xQualTH = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER iIMSHO1xQualTH to: %d",
                        qmi_req.iIMSHO1xQualTH);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WIFI_THRESHOLDA:
        qmi_req.iIMSHOWIFIQualTHA_valid = TRUE;
        qmi_req.iIMSHOWIFIQualTHA = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER iIMSHOWIFIQualTHA to: %d",
                        qmi_req.iIMSHOWIFIQualTHA);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WIFI_THRESHOLDB:
        qmi_req.iIMSHOWIFIQualTHB_valid = TRUE;
        qmi_req.iIMSHOWIFIQualTHB = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER iIMSHOWIFIQualTHB to: %d",
                        qmi_req.iIMSHOWIFIQualTHB);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WLAN_TO_WWAN_HYS_TIMER:
        qmi_req.wlan_to_wwan_hys_timer_valid = TRUE;
        qmi_req.wlan_to_wwan_hys_timer = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER wlan_to_wwan_hys_timer to: %d",
                        qmi_req.wlan_to_wwan_hys_timer);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WLAN_TO_1X_HYS_TIMER:
        qmi_req.wlan_to_1x_hys_timer_valid = TRUE;
        qmi_req.wlan_to_1x_hys_timer = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER wlan_to_1x_hys_timer to: %d",
                        qmi_req.wlan_to_1x_hys_timer);
        break;

      case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WWAN_TO_WLAN_HYS_TIMER:
        qmi_req.wwan_to_wlan_hys_timer_valid = TRUE;
        qmi_req.wwan_to_wlan_hys_timer = *((int *)config_ptr->config_item_value);
        QCRIL_LOG_INFO("Set config HANDOVER wwan_to_wlan_hys_timer to: %d",
                        qmi_req.wwan_to_wlan_hys_timer);
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
          QMI_IMS_SETTINGS_SET_HANDOVER_CONFIG_REQ_V01,
          &qmi_req,
          sizeof(ims_settings_set_handover_config_req_msg_v01),
          sizeof(ims_settings_set_handover_config_rsp_msg_v01),
          qcril_qmi_imss_config_resp_cb_v02,
          config_ptr->extra_data);

      QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
      radio_config_error =
        qcril_qmi_radio_config_map_internal_error_to_radio_config_error(qmi_client_error);
    }

  }while(FALSE);

  QCRIL_LOG_FUNC_RETURN_WITH_RET(radio_config_error);
  return radio_config_error;
}//qcril_qmi_radio_config_imss_set_handover_config_req_handler

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
// qcril_qmi_radio_config_imss_get_sip_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_sip_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_sip_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

  //place holder for copying the values.
  uint32_t int_value = 0;
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
    config_params.config_item_value_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_sip_config_rsp_msg_v01 *) config_ptr->extra_data;
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
        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_OPERATOR_MODE_A:
          if(qmi_resp->sip_timer_operator_mode_a_valid)
          {
            int_value = qmi_resp->sip_timer_operator_mode_a;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP sip_timer_operator_mode_a value: %d",
                            qmi_resp->sip_timer_operator_mode_a);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T1:
          if(qmi_resp->timer_t1_valid)
          {
            int_value = qmi_resp->timer_t1;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_t1 value: %d",
                            qmi_resp->timer_t1);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T2:
          if(qmi_resp->timer_t2_valid)
          {
            int_value = qmi_resp->timer_t2;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_t2 value: %d",
                            qmi_resp->timer_t2);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TF:
          if(qmi_resp->timer_tf_valid)
          {
            int_value = qmi_resp->timer_tf;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_tf value: %d",
                            qmi_resp->timer_tf);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;


        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_T4:
          if(qmi_resp->timer_t4_valid)
          {
            int_value = qmi_resp->timer_t4;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_t4 value: %d",
                            qmi_resp->timer_t4);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TA_VALUE:
          if(qmi_resp->timer_ta_value_valid)
          {
            int_value = qmi_resp->timer_ta_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_ta_value value: %d",
                            qmi_resp->timer_ta_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TB_VALUE:
          if(qmi_resp->timer_tb_value_valid)
          {
            int_value = qmi_resp->timer_tb_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_tb_value value: %d",
                            qmi_resp->timer_tb_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TD_VALUE:
          if(qmi_resp->timer_td_value_valid)
          {
            int_value = qmi_resp->timer_td_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_td_value value: %d",
                            qmi_resp->timer_td_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TE_VALUE:
          if(qmi_resp->timer_te_value_valid)
          {
            int_value = qmi_resp->timer_te_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_te_value value: %d",
                            qmi_resp->timer_te_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TG_VALUE:
          if(qmi_resp->timer_tg_value_valid)
          {
            int_value = qmi_resp->timer_tg_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_tg_value value: %d",
                            qmi_resp->timer_tg_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TH_VALUE:
          if(qmi_resp->timer_th_value_valid)
          {
            int_value = qmi_resp->timer_th_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_th_value value: %d",
                            qmi_resp->timer_th_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TI_VALUE:
          if(qmi_resp->timer_ti_value_valid)
          {
            int_value = qmi_resp->timer_ti_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_ti_value value: %d",
                            qmi_resp->timer_ti_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TJ:
          if(qmi_resp->timer_tj_valid)
          {
            int_value = qmi_resp->timer_tj;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_tj value: %d",
                            qmi_resp->timer_tj);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_TIMER_TK_VALUE:
          if(qmi_resp->timer_tk_value_valid)
          {
            int_value = qmi_resp->timer_tk_value;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get SIP timer_tk_value value: %d",
                            qmi_resp->timer_tk_value);
          }
          else
          {
              QCRIL_LOG_INFO("Did not get the value requested in qmi message");
              radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_SIP_KEEPALIVE_ENABLED:
          if(qmi_resp->keepalive_enabled_valid)
          {
            bool_value = (qmi_resp->keepalive_enabled) ? TRUE : FALSE;
            config_params.config_item_value_len = sizeof(bool_value);
            config_params.config_item_value = (void*) &bool_value;
            QCRIL_LOG_INFO("get SIP keepalive_enabled value: %d",
                            qmi_resp->keepalive_enabled);
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
}// qcril_qmi_radio_config_imss_get_sip_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_get_handover_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_handover_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_get_handover_config_rsp_msg_v01 *qmi_resp = NULL;
  qcril_qmi_ims_config_error_type radio_config_error =
        QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  qcril_qmi_radio_config_params_type config_params;
  memset(&config_params, 0, sizeof(config_params));

  /* To check if error cause code is sent in FAILURE case */
  bool error_cause_code_present = FALSE;

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
    config_params.config_item_value_type =
        qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

    /* Get the qmi response structure from req_params_type */
    qmi_resp = (ims_settings_get_handover_config_rsp_msg_v01 *) config_ptr->extra_data;
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
        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD1:
          if(qmi_resp->iIMSHOLTEQualTH1_valid)
          {
            int_value = qmi_resp->iIMSHOLTEQualTH1;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER iIMSHOLTEQualTH1 value: %d",
                            qmi_resp->iIMSHOLTEQualTH1);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD2:
          if(qmi_resp->iIMSHOLTEQualTH2_valid)
          {
            int_value = qmi_resp->iIMSHOLTEQualTH2;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER iIMSHOLTEQualTH2 value: %d",
                            qmi_resp->iIMSHOLTEQualTH2);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_LTE_THRESHOLD3:
          if(qmi_resp->iIMSHOLTEQualTH3_valid)
          {
            int_value = qmi_resp->iIMSHOLTEQualTH3;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER iIMSHOLTEQualTH3 value: %d",
                            qmi_resp->iIMSHOLTEQualTH3);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_1X_THRESHOLD:
          if(qmi_resp->iIMSHO1xQualTH_valid)
          {
            int_value = qmi_resp->iIMSHO1xQualTH;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER iIMSHO1xQualTH value: %d",
                            qmi_resp->iIMSHO1xQualTH);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WIFI_THRESHOLDA:
          if(qmi_resp->iIMSHOWIFIQualTHA_valid)
          {
            int_value = qmi_resp->iIMSHOWIFIQualTHA;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER iIMSHOWIFIQualTHA value: %d",
                            qmi_resp->iIMSHOWIFIQualTHA);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WIFI_THRESHOLDB:
          if(qmi_resp->iIMSHOWIFIQualTHB_valid)
          {
            int_value = qmi_resp->iIMSHOWIFIQualTHB;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER iIMSHOWIFIQualTHB value: %d",
                            qmi_resp->iIMSHOWIFIQualTHB);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WLAN_TO_WWAN_HYS_TIMER:
          if(qmi_resp->wlan_to_wwan_hys_timer_valid)
          {
            int_value = qmi_resp->wlan_to_wwan_hys_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER wlan_to_wwan_hys_timer value: %d",
                            qmi_resp->wlan_to_wwan_hys_timer);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WLAN_TO_1X_HYS_TIMER:
          if(qmi_resp->wlan_to_1x_hys_timer_valid)
          {
            int_value = qmi_resp->wlan_to_1x_hys_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER wlan_to_1x_hys_timer value: %d",
                            qmi_resp->wlan_to_1x_hys_timer);
          }
          else
          {
            QCRIL_LOG_INFO("Did not get the value requested in qmi message");
            radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_ITEM_NOT_PRESENT;
          }
          break;

        case QCRIL_QMI_RADIO_CONFIG_HANDOVER_CONFIG_WWAN_TO_WLAN_HYS_TIMER:
          if(qmi_resp->wwan_to_wlan_hys_timer_valid)
          {
            int_value = qmi_resp->wwan_to_wlan_hys_timer;
            config_params.config_item_value_len = sizeof(int_value);
            config_params.config_item_value = (void*) &int_value;
            QCRIL_LOG_INFO("get HANDOVER wwan_to_wlan_hys_timer value: %d",
                            qmi_resp->wwan_to_wlan_hys_timer);
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
    /* If not any of the above, ril_err is generic error and
    * Empty payload response is sent back */
    else
    {
      QCRIL_LOG_INFO("qmi error response");
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(
              qmi_resp->resp.error);
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
}//qcril_qmi_radio_config_imss_get_handover_config_resp_handler

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
// qcril_qmi_radio_config_imss_set_sip_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_sip_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_sip_config_rsp_msg_v01 *qmi_resp = NULL;
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
    qmi_resp = (ims_settings_set_sip_config_rsp_msg_v01 *) config_ptr->extra_data;

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
}// qcril_qmi_radio_config_imss_set_sip_config_resp_handler

//===========================================================================
// qcril_qmi_radio_config_imss_set_handover_config_resp_handler
//===========================================================================
qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_handover_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
)
{
  QCRIL_LOG_FUNC_ENTRY();

  ims_settings_set_handover_config_rsp_msg_v01 *qmi_resp = NULL;
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
    qmi_resp = (ims_settings_set_handover_config_rsp_msg_v01 *) config_ptr->extra_data;

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
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(
          qmi_resp->resp.error);
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
      radio_config_error = qcril_qmi_radio_config_map_qmi_error_to_radio_config_error(
          qmi_resp->resp.error);
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
}// qcril_qmi_radio_config_imss_set_handover_config_resp_handler

//==============================================================================
// qcril_qmi_radio_config_imss_map_radio_config_video_quality_to_imss_vt_quality
//==============================================================================
boolean qcril_qmi_radio_config_imss_map_radio_config_video_quality_to_imss_vt_quality
(
  qcril_qmi_radio_config_imss_video_quality radio_config_video_quality,
  ims_settings_qipcall_vt_quality_enum_v01 *ims_vt_quality
)
{
  boolean ret = FALSE;
  if (ims_vt_quality)
  {
    ret = TRUE;
    switch(radio_config_video_quality)
    {
      case QCRIL_QMI_RADIO_CONFIG_IMSS_VIDEO_QUALITY_LOW:
        *ims_vt_quality = IMS_SETTINGS_VT_QUALITY_LEVEL_2_V01;
        break;
      case QCRIL_QMI_RADIO_CONFIG_IMSS_VIDEO_QUALITY_MID:
        *ims_vt_quality = IMS_SETTINGS_VT_QUALITY_LEVEL_1_V01;
        break;
      case QCRIL_QMI_RADIO_CONFIG_IMSS_VIDEO_QUALITY_HIGH:
        *ims_vt_quality = IMS_SETTINGS_VT_QUALITY_LEVEL_0_V01;
        break;
      default:
        ret = FALSE;
        break;
    }
    QCRIL_LOG_INFO("Mapped radio config video quality %d, to ims vt quality %d",
          radio_config_video_quality, *ims_vt_quality);
  }
  else
  {
    QCRIL_LOG_INFO("Invalid parameters");
  }
  return ret;
}

//==============================================================================
// qcril_qmi_radio_config_imss_map_imss_vt_quality_to_radio_config_video_quality
//==============================================================================
boolean qcril_qmi_radio_config_imss_map_imss_vt_quality_to_radio_config_video_quality
(
  ims_settings_qipcall_vt_quality_enum_v01   ims_vt_quality,
  qcril_qmi_radio_config_imss_video_quality *radio_config_video_quality
)
{
  boolean ret = FALSE;
  if (radio_config_video_quality)
  {
    ret = TRUE;
    switch(ims_vt_quality)
    {
      case IMS_SETTINGS_VT_QUALITY_LEVEL_0_V01:
        *radio_config_video_quality = QCRIL_QMI_RADIO_CONFIG_IMSS_VIDEO_QUALITY_HIGH;
        break;
      case IMS_SETTINGS_VT_QUALITY_LEVEL_1_V01:
        *radio_config_video_quality = QCRIL_QMI_RADIO_CONFIG_IMSS_VIDEO_QUALITY_MID;
        break;
      case IMS_SETTINGS_VT_QUALITY_LEVEL_2_V01:
        *radio_config_video_quality = QCRIL_QMI_RADIO_CONFIG_IMSS_VIDEO_QUALITY_LOW;
        break;
      default:
        ret = FALSE;
        break;
    }
    QCRIL_LOG_INFO("Mapped ims vt quality: %d, to radio config video quality: %d",
            ims_vt_quality, *radio_config_video_quality);
  }
  else
  {
      QCRIL_LOG_INFO("Invalid parameters");
  }
  return ret;
}
