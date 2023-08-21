#ifdef FEATURE_SUPPORT_IMSS_DEPRECATED
/******************************************************************************
#  Copyright (c) 2013-2015, 2017, 2019-2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/******************************************************************************
  @file    qcril_qmi_imss.c
  @brief   qcril qmi - IMS Setting

  DESCRIPTION
    Handles RIL requests, Callbacks, indications for QMI IMS Setting.

******************************************************************************/

#include <cutils/properties.h>

#define TAG "QCRIL_IMSS"

#include "qcril_log.h"
#include <framework/Log.h>
#include "qcril_qmi_client.h"
#include "../qcril_qmi_imss.h"
#include "ip_multimedia_subsystem_settings_v01.h"
#include "qcril_reqlist.h"
#include "qcril_qmi_npb_utils.h"
#include "qcril_qmi_radio_config_imss.h"
#include "modules/nas/qcril_qmi_nas.h"
#include "qcril_qmi_imss_v02.h"
#include "modules/nas/qcril_arb.h"

#include "qcril_qmi_imss_qmi.h"
#include "modules/nas/NasSetVoiceDomainPreferenceRequest.h"
#include "SetRatPrefMessage.h"

#include <interfaces/ims/ims.h>
#include "modules/ims/ImsModule.h"
#include "qcril_qmi_ims_utils.h"

extern struct ims_cached_info_type   qcril_qmi_ims_cached_info;
extern boolean feature_voice_dom_pref_on_toggle_ims_cap;
extern boolean feature_disabled_modem_req;

static ims_settings_wfc_status_enum_v01 qcril_qmi_imss_convert_ril_to_imss_wfc_status
(
  qcril_ims_setting_wfc_status_type wfc_status
);

static ims_settings_wfc_preference_v01 qcril_qmi_imss_convert_ril_to_imss_wfc_preference
(
  qcril_ims_setting_wfc_preference_type wfc_preference
);

static RIL_Errno qcril_qmi_imss_send_set_qipcall_config_req
(
 const ims_settings_set_qipcall_config_req_msg_v01 *qmi_req,
 uint32 user_data
);

//===========================================================================
// qcril_qmi_imss_set_reg_mgr_config_resp_hdlr
//===========================================================================
void qcril_qmi_imss_set_reg_mgr_config_resp_hdlr
(
 unsigned int                 msg_id,
 std::shared_ptr<void>        resp_c_struct,
 unsigned int                 resp_c_struct_len,
 void                        *resp_cb_data,
 qmi_client_error_type        transp_err
)
{
  (void)msg_id;
  (void)resp_c_struct_len;
  ims_settings_set_reg_mgr_config_rsp_msg_v01 *resp;
  RIL_Errno ril_err = RIL_E_GENERIC_FAILURE;
  uint32 user_data = (uint32)(uintptr_t) resp_cb_data;
  uint16_t req_id = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);

  QCRIL_LOG_FUNC_ENTRY();

  getImsModule()->getPendingMessageList().print();
  auto pendingMsg = getImsModule()->getPendingMessageList().extract(req_id);
  getImsModule()->getPendingMessageList().print();

  do {
    if (pendingMsg == nullptr)
    {
      QCRIL_LOG_ERROR("pendingMsg is null");
      break;
    }
    if (transp_err != QMI_NO_ERR)
    {
      QCRIL_LOG_ERROR("Transp error (%d) recieved from QMI", transp_err);
      ril_err = RIL_E_GENERIC_FAILURE;
      break;
    }
    resp = (ims_settings_set_reg_mgr_config_rsp_msg_v01*)(resp_c_struct.get());
    if (resp == nullptr)
    {
      QCRIL_LOG_ERROR("resp is null");
      ril_err = RIL_E_GENERIC_FAILURE;
      break;
    }
    ril_err = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR, &(resp->resp));
    QCRIL_LOG_ESSENTIAL("ril_err: %d, qmi res: %d", (int) ril_err, (int)resp->resp.error);
  } while (FALSE);

  if (pendingMsg != nullptr)
  {
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(ril_err, nullptr);
    auto msg(std::static_pointer_cast<QcRilRequestImsRegistrationChangeMessage>(pendingMsg));
    msg->sendResponse(pendingMsg, Message::Callback::Status::SUCCESS, respPayload);
  }

  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imss_set_reg_mgr_config_resp_hdlr

//===========================================================================
// qcril_qmi_imss_set_qipcall_config_resp_hdlr
//===========================================================================
void qcril_qmi_imss_set_qipcall_config_resp_hdlr
(
   unsigned int                 msg_id,
   std::shared_ptr<void>        resp_c_struct,
   unsigned int                 resp_c_struct_len,
   void                        *resp_cb_data,
   qmi_client_error_type        transp_err
)
{
  (void)msg_id;
  (void)resp_c_struct_len;
  ims_settings_set_qipcall_config_rsp_msg_v01 *resp;
  RIL_Errno ril_err = RIL_E_GENERIC_FAILURE;

  uint32 user_data = (uint32)(uintptr_t) resp_cb_data;
  uint16_t req_id = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
  auto pendingMsg = getImsModule()->getPendingMessageList().find(req_id);

  QCRIL_LOG_FUNC_ENTRY();

  do {
    if (pendingMsg == nullptr)
    {
      QCRIL_LOG_ERROR("pendingMsg is null");
      break;
    }
    if (transp_err != QMI_NO_ERR)
    {
      QCRIL_LOG_ERROR("Transp error (%d) recieved from QMI", transp_err);
      break;
    }
    resp = (ims_settings_set_qipcall_config_rsp_msg_v01*)(resp_c_struct.get());
    if (resp == nullptr)
    {
      QCRIL_LOG_ERROR("resp is null");
      break;
    }
    ril_err = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR, &(resp->resp));
    QCRIL_LOG_ESSENTIAL("ril_err: %d, qmi res: %d", (int) ril_err, (int)resp->resp.error);
    if (pendingMsg->get_message_id() ==
        QcRilRequestImsSetServiceStatusMessage::get_class_message_id()) {
        //TODO
        qcril_qmi_imss_handle_ims_set_service_status_event_resp(req_id, ril_err);
    } else {
      //TODO enable based on request
      //auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(ril_err, nullptr);
      //pendingMsg->sendResponse(pendingMsg, Message::Callback::Status::SUCCESS, respPayload);
      //erase the message
      //getImsModule()->getPendingMessageList().erase(pendingMsg);

    }
  } while(FALSE);

  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imss_set_qipcall_config_resp_hdlr

//===========================================================================
// qcril_qmi_imss_set_client_provisioning_config_resp_hdlr
//===========================================================================
void qcril_qmi_imss_set_client_provisioning_config_resp_hdlr
(
   unsigned int                 msg_id,
   std::shared_ptr<void>        resp_c_struct,
   unsigned int                 resp_c_struct_len,
   void                        *resp_cb_data,
   qmi_client_error_type        transp_err
)
{
    (void)msg_id;
    (void)resp_c_struct_len;
    ims_settings_set_client_provisioning_config_rsp_msg_v01 *resp;
    RIL_Errno ril_err = RIL_E_GENERIC_FAILURE;

    uint32 user_data = (uint32)(uintptr_t) resp_cb_data;
    uint16_t req_id = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
    auto pendingMsg = getImsModule()->getPendingMessageList().extract(req_id);

    QCRIL_LOG_FUNC_ENTRY();

    do {
        if (pendingMsg == nullptr)
        {
            QCRIL_LOG_ERROR("pendingMsg is null");
            break;
        }
        if (transp_err != QMI_NO_ERR)
        {
            QCRIL_LOG_ERROR("Transp error (%d) recieved from QMI", transp_err);
            break;
        }
        resp = (ims_settings_set_client_provisioning_config_rsp_msg_v01*)(resp_c_struct.get());
        if (resp == nullptr)
        {
            QCRIL_LOG_ERROR("resp is null");
            break;
        }
        ril_err = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(
                QMI_NO_ERR, &(resp->resp));
        QCRIL_LOG_ESSENTIAL("ril_err: %d, qmi res: %d", (int) ril_err, (int)resp->resp.error);

    } while (FALSE);

    if (pendingMsg) {
        auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(ril_err, nullptr);
        auto ril_msg = std::static_pointer_cast<QcRilRequestImsSetServiceStatusMessage>(pendingMsg);
        ril_msg->sendResponse(ril_msg, Message::Callback::Status::SUCCESS, respPayload);
    }

    QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imss_set_client_provisioning_config_resp_hdlr



//===========================================================================
// qcril_qmi_imss_request_set_ims_registration
//===========================================================================
void qcril_qmi_imss_request_set_ims_registration(
    std::shared_ptr<QcRilRequestImsRegistrationChangeMessage> msg)
{
  qcril::interfaces::RegState imsRegState = qcril::interfaces::RegState::UNKNOWN;
  ims_settings_set_reg_mgr_config_req_msg_v01 qmi_req;
  bool sendResponse = false;
  uint32 user_data;
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;
  RIL_Errno errorCode = RIL_E_SUCCESS;
  uint16_t req_id = -1;
  bool doCleanup = false;

  QCRIL_LOG_FUNC_ENTRY();

  do
  {
    imsRegState = msg->getRegState();
    //QCRIL_LOG_INFO("state: %d", imsRegState);

    if (feature_disabled_modem_req)
    {
      QCRIL_LOG_INFO("Modem IMS config is disabled. Respond to Telephony with success");
      errorCode = RIL_E_SUCCESS;
      sendResponse = true;
      break;
    }

    if(imsRegState == qcril::interfaces::RegState::UNKNOWN)
    {
      QCRIL_LOG_ERROR("Invalid parameters: state is not present; Send failure");
      errorCode = RIL_E_GENERIC_FAILURE;
      sendResponse = true;
      break;
    }

    getImsModule()->getPendingMessageList().print();

    auto pendingMsgStatus = getImsModule()->getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      break;
    }
    req_id = pendingMsgStatus.first;
    user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID, req_id);

    // Call QMI IMSS API to set the ims test mode.
    memset(&qmi_req, 0, sizeof(qmi_req));
    qmi_req.ims_test_mode_enabled_valid = TRUE;
    qmi_req.ims_test_mode_enabled =
        (imsRegState == qcril::interfaces::RegState::REGISTERED) ? FALSE : TRUE;
    QCRIL_LOG_INFO("ims_test_mode_enabled = %s", qmi_req.ims_test_mode_enabled ? "TRUE" : "FALSE");

    qmi_client_error = qmi_client_imss_send_async(
        QMI_IMS_SETTINGS_SET_REG_MGR_CONFIG_REQ_V01,
        &qmi_req,
        sizeof(ims_settings_set_reg_mgr_config_req_msg_v01),
        sizeof(ims_settings_set_reg_mgr_config_rsp_msg_v01),
        qcril_qmi_imss_set_reg_mgr_config_resp_hdlr,
        (void*)(uintptr_t)user_data);
    QCRIL_LOG_INFO(".. qmi send async res %d", (int) qmi_client_error);

    if (qmi_client_error != QMI_NO_ERR)
    {
      errorCode = RIL_E_GENERIC_FAILURE;
      sendResponse = true;
      doCleanup = true;
      break;
    }
  } while (FALSE);

  if (sendResponse)
  {
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(errorCode, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
    if (doCleanup) {
      getImsModule()->getPendingMessageList().erase(req_id);
    }
  }
  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imss_request_set_ims_registration

//===========================================================================
// qcril_qmi_imss_request_set_ims_srv_status
//===========================================================================
void qcril_qmi_imss_request_set_ims_srv_status(
    std::shared_ptr<QcRilRequestImsSetServiceStatusMessage> msg) {
  RIL_Errno res = RIL_E_GENERIC_FAILURE;

  QCRIL_LOG_FUNC_ENTRY();

  if (msg != nullptr) {
    do {
      if (feature_disabled_modem_req) {
        QCRIL_LOG_INFO("Modem IMS config is disabled. Respond to Telephony with success");
        res = RIL_E_SUCCESS;
        break;
      }
      if (msg->hasVolteEnabled() && msg->hasVideoTelephonyEnabled() &&
          msg->hasWifiCallingEnabled()) {
        QCRIL_LOG_ERROR("Not supported settings all volte/vt/wfc in a single request");
        res = RIL_E_REQUEST_NOT_SUPPORTED;
        break;
      }
      auto pendingMsgStatus = getImsModule()->getPendingMessageList().insert(msg);
      if (pendingMsgStatus.second != true) {
        break;
      }
      uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                                 pendingMsgStatus.first);
      if (msg->hasWifiCallingEnabled()) {
        // Set the wifi calling setting through client provisioning settings.
        // If the radio tech is WLAN. So declare the variable.
        ims_settings_set_client_provisioning_config_req_msg_v01 qmi_client_provisioning_req = {};
        qmi_client_provisioning_req.wifi_call_valid = TRUE;
        qmi_client_provisioning_req.wifi_call = msg->getWifiCallingEnabled()
                                                    ? IMS_SETTINGS_WFC_STATUS_ON_V01
                                                    : IMS_SETTINGS_WFC_STATUS_OFF_V01;
        QCRIL_LOG_INFO("Sending wifi call setting through set_service, value: %d",
                       qmi_client_provisioning_req.wifi_call);

        qmi_client_error_type qmi_client_error = qmi_client_imss_send_async(
            QMI_IMS_SETTINGS_SET_CLIENT_PROVISIONING_CONFIG_REQ_V01, &qmi_client_provisioning_req,
            sizeof(ims_settings_set_client_provisioning_config_req_msg_v01),
            sizeof(ims_settings_set_client_provisioning_config_rsp_msg_v01),
            qcril_qmi_imss_set_client_provisioning_config_resp_hdlr, (void*)(uintptr_t)user_data);
        QCRIL_LOG_INFO(".. qmi send async res %d", (int)qmi_client_error);
        if (qmi_client_error == E_SUCCESS) {
          res = RIL_E_SUCCESS;
        }
      }
      ims_settings_set_qipcall_config_req_msg_v01 qmi_req = {};
      if (msg->hasVolteEnabled()) {
        qmi_req.volte_enabled_valid = TRUE;
        qmi_req.volte_enabled = msg->getVolteEnabled();
      }
      if (msg->hasVideoTelephonyEnabled()) {
        qmi_req.vt_calling_enabled_valid = TRUE;
        qmi_req.vt_calling_enabled = msg->getVideoTelephonyEnabled();
      }
      if (feature_voice_dom_pref_on_toggle_ims_cap && qmi_req.volte_enabled_valid &&
          qmi_req.volte_enabled) {
        // Make sure to send the response only when we set voice_domain_preference and
        // volte_enabled.
        auto setVoiceDomainPrefMsg = std::make_shared<NasSetVoiceDomainPreferenceRequest>(
            NAS_VOICE_DOMAIN_PREF_PS_PREF_V01,
            [msg, qmi_req, user_data](std::shared_ptr<Message> /*msg*/,
                                      Message::Callback::Status status,
                                      std::shared_ptr<RIL_Errno> result) -> void {
              RIL_Errno res = RIL_E_GENERIC_FAILURE;
              if (status == Message::Callback::Status::SUCCESS && result) {
                res = *result;
              }
              QCRIL_LOG_DEBUG("NasSetVoiceDomainPreferenceRequest: result = %d", res);
              if (res == RIL_E_SUCCESS) {
                res = qcril_qmi_imss_send_set_qipcall_config_req(&qmi_req, user_data);
              } else {
                msg->setPendingMessageState(PendingMessageState::COMPLETED_FAILURE);
              }
              if (res != RIL_E_SUCCESS) {
                msg->sendResponse(
                    msg, Message::Callback::Status::SUCCESS,
                    std::make_shared<QcRilRequestMessageCallbackPayload>(res, nullptr));
                getImsModule()->getPendingMessageList().erase(msg);
              }
            });
        if (setVoiceDomainPrefMsg) {
          setVoiceDomainPrefMsg->dispatch();
          res = RIL_E_SUCCESS;
        }
        break;
      }
      if (qmi_req.volte_enabled_valid || qmi_req.vt_calling_enabled_valid) {
        res = qcril_qmi_imss_send_set_qipcall_config_req(&qmi_req, user_data);
      } else {
        QCRIL_LOG_ERROR("request misses some necessary information");
        break;
      }
    } while (FALSE);
    if (feature_disabled_modem_req || RIL_E_SUCCESS != res) {
      auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(res, nullptr);
      msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
      getImsModule()->getPendingMessageList().erase(msg);
    }
  }

  QCRIL_LOG_FUNC_RETURN();
}  // qcril_qmi_imss_request_set_ims_srv_status

//===========================================================================
// qcril_qmi_imss_request_set_ims_config
//
// Functionality:
// 1. Reads config item from the ims request
// 2. creates a reqlist entry (store the config item id here)
// 3. Depending on the type of item's value, process the data
// 4. and Store the processed data in Config params
// 5. gets the set request handler for the config item
// 6. Call the handler with config params,
//    to send down the appropriate req
//===========================================================================
void qcril_qmi_imss_request_set_ims_config
(
  std::shared_ptr<QcRilRequestImsSetConfigMessage> msg
)
{
  uint32 user_data;
  qcril_instance_id_e_type instance_id = QCRIL_DEFAULT_INSTANCE_ID;

  /* Radio config mapping related types.
   Needed for processing the request*/
  qcril_qmi_radio_config_params_type config_params;
  qcril_qmi_ims_config_item_value_type config_item_type;
  qcril_qmi_radio_config_handler_type* req_handler = NULL;
  qcril_qmi_ims_config_error_type radio_config_error =
    QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;

  /* To Convert the int value to bool  */
  bool bool_value;
  bool sendResponse = false;
  uint16_t req_id = 0;
  uint32_t intVal = 0;
  std::string strVal;

  QCRIL_LOG_FUNC_ENTRY();
  if (msg != nullptr) {
    do {
      if(feature_disabled_modem_req)
      {
        QCRIL_LOG_INFO("Modem IMS config is disabled. Respond to Telephony with success");
        radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS;
        auto config = std::make_shared<qcril::interfaces::ConfigInfo>();
        if (config) {
          config->setItem(msg->getConfigInfo().getItem());
        }
        auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(
                                RIL_E_SUCCESS, config);
        msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
        break;
      }

      auto pendingMsgStatus = getImsModule()->getPendingMessageList().insert(msg);
      if (pendingMsgStatus.second != true)
      {
          sendResponse = true;
          break;
      }
      req_id = pendingMsgStatus.first;

      qcril::interfaces::ConfigInfo &configInfo = msg->getConfigInfo();
      if (!configInfo.hasItem()) {
        sendResponse = true;
        QCRIL_LOG_ERROR("Config Item is not present in the request, ..sending failure");
        break;
      }
      config_params.config_item =
        qcril_qmi_ims_map_ims_config_to_radio_config_item(configInfo.getItem());

      if(config_params.config_item == QCRIL_QMI_IMS_CONFIG_INVALID)
      {
        QCRIL_LOG_ERROR("Invalid config item: %d. Doesnt exist in radio config",
                         configInfo.getItem());
        sendResponse = true;
        break;
      }

      user_data = QCRIL_COMPOSE_USER_DATA( instance_id,
                                           QCRIL_DEFAULT_MODEM_ID,
                                           req_id );

      QCRIL_LOG_INFO("processing request - config item: %s",
                   qcril_qmi_radio_config_get_item_log_str(config_params.config_item));
      config_item_type = qcril_qmi_radio_config_get_item_value_type(config_params.config_item);

      // If item value type is boolean and input params has int val
      // As per imsproto for now the items coming in Boolean are placed
      // in intValue as there is no corresponding API in ImsConfig.java
      if((config_item_type == QCRIL_QMI_RADIO_CONFIG_ITEM_IS_BOOLEAN)
          && (configInfo.hasIntValue()))
      {
        QCRIL_LOG_INFO("Config item received is of boolean type in intValue, value: %d",
                        configInfo.getIntValue());
        bool_value = (configInfo.getIntValue() ? TRUE : FALSE );
        config_params.config_item_value_len = sizeof(bool_value);
        config_params.config_item_value = (void *)(&bool_value);
      }
      else if ((config_item_type == QCRIL_QMI_RADIO_CONFIG_ITEM_IS_INT)
                           && configInfo.hasIntValue()) {
        QCRIL_LOG_INFO("Config item received is an integer, value: %d",
                        configInfo.getIntValue());
        intVal = configInfo.getIntValue();
        config_params.config_item_value_len = sizeof(intVal);
        config_params.config_item_value = (void *)(&intVal);
      }
      else if((config_item_type == QCRIL_QMI_RADIO_CONFIG_ITEM_IS_STRING)
                          && (configInfo.hasStringValue())) {
        strVal = configInfo.getStringValue();
        config_params.config_item_value_len = strlen(strVal.c_str());
        QCRIL_LOG_INFO("Config item received is a string, value: %s", strVal.c_str());
        config_params.config_item_value = (void *) strVal.c_str();
        //TODO check string
      }
      else
      {
         QCRIL_LOG_ERROR("..invalid parameters for the config items value");
         radio_config_error = QCRIL_QMI_RADIO_CONFIG_ERROR_INVALID_PARAMETER;
         sendResponse = true;
         break;
      }
      config_params.extra_data_len = sizeof(user_data);
      config_params.extra_data = (void *)(uintptr_t)user_data;
      config_params.config_item_value_type = config_item_type;
      /* Get the set request handler and call it with config params */
      req_handler = qcril_qmi_radio_config_find_set_config_req_handler(config_params.config_item);

      if(req_handler == NULL)
      {
        QCRIL_LOG_ERROR("NULL req handler for the item");
        sendResponse = true;
        break;
      }
      radio_config_error = (req_handler)(&config_params, req_id);
      QCRIL_LOG_INFO("Returned from req handler with radio_config_error: %d", radio_config_error);
    }while (FALSE);
    if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      QCRIL_LOG_ERROR("radio config error: %d..sending empty response", radio_config_error);
      qcril_qmi_imss_set_ims_config_log_and_send_response(req_id,
                                                      NULL,
                                                      radio_config_error,
                                                      QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
    }
  }

  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imss_request_set_ims_config

//===========================================================================
// qcril_qmi_imss_request_get_ims_config
//
// Functionality:
// 1. Reads config item from the ims request
// 2. creates a reqlist entry (store the config item id here)
// 3. Store the processed data in Config params
// 5. gets the get request handler for the config item
// 6. Call the handler with config params,
//    to send down the appropriate req
//===========================================================================
void qcril_qmi_imss_request_get_ims_config
(
    std::shared_ptr<QcRilRequestImsGetConfigMessage> msg
)
{
  uint32 user_data;
  qcril_instance_id_e_type instance_id = QCRIL_DEFAULT_INSTANCE_ID;
   /* Radio config mapping related types. */
  qcril_qmi_radio_config_params_type config_params;
  qcril_qmi_radio_config_handler_type* req_handler = NULL;
  qcril_qmi_ims_config_error_type radio_config_error =
    QCRIL_QMI_RADIO_CONFIG_ERROR_GENERIC_FAILURE;
  uint16_t req_id = 0;
  QCRIL_LOG_FUNC_ENTRY();
  if (msg != nullptr) {
    do {
      auto pendingMsgStatus = getImsModule()->getPendingMessageList().insert(msg);
      if (pendingMsgStatus.second != true)
      {
          break;
      }
      req_id = pendingMsgStatus.first;

      qcril::interfaces::ConfigInfo &configInfo = msg->getConfigInfo();
      if (!configInfo.hasItem()) {
        QCRIL_LOG_ERROR("Config Item is not present in the request, ..sending failure");
        break;
      }
      config_params.config_item =
        qcril_qmi_ims_map_ims_config_to_radio_config_item(configInfo.getItem());

      if(config_params.config_item == QCRIL_QMI_IMS_CONFIG_INVALID)
      {
        QCRIL_LOG_ERROR("Invalid config item: %d. Doesnt exist in radio config",
                         configInfo.getItem());
        break;
      }

      user_data = QCRIL_COMPOSE_USER_DATA( instance_id,
                                           QCRIL_DEFAULT_MODEM_ID,
                                           req_id );
      QCRIL_LOG_INFO("processing request - config item: %s",
                   qcril_qmi_radio_config_get_item_log_str(config_params.config_item));

      config_params.config_item_value_len = 0;
      config_params.config_item_value = NULL;
      config_params.extra_data_len = sizeof(user_data);
      config_params.extra_data = (void *)(uintptr_t)user_data;

      /* Get the get request handler and call it with config params */
      req_handler = qcril_qmi_radio_config_find_get_config_req_handler(config_params.config_item);
      if(req_handler == NULL)
      {
        QCRIL_LOG_ERROR("NULL req handler for the item");
        break;
      }

      radio_config_error = (req_handler)(&config_params, req_id);

      QCRIL_LOG_INFO("Returned from req handler with radio_config_error: %d",
                      radio_config_error);
    } while(FALSE);

    if(radio_config_error != QCRIL_QMI_RADIO_CONFIG_ERROR_SUCCESS)
    {
      QCRIL_LOG_ERROR("radio config error: %d..sending empty response", radio_config_error);
      qcril_qmi_imss_get_ims_config_log_and_send_response(req_id,
                                                      NULL,
                                                      radio_config_error,
                                                      QCRIL_QMI_RADIO_CONFIG_SETTINGS_RESP_NO_ERR);
    }
  }
  QCRIL_LOG_FUNC_RETURN();
} //qcril_qmi_imss_request_get_ims_config


 //===========================================================================
// qcril_qmi_imss_config_resp_cb
//===========================================================================
void qcril_qmi_imss_config_resp_cb
(
   unsigned int                 msg_id,
   std::shared_ptr<void>        resp_c_struct,
   unsigned int                 resp_c_struct_len,
   void                        *resp_cb_data,
   qmi_client_error_type        transp_err
)
{
 (void)msg_id;
 (void)resp_c_struct_len;
 int config_item;

 uint32 user_data = (uint32)(uintptr_t) resp_cb_data;
 uint16_t req_id = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
 auto pendingMsg = getImsModule()->getPendingMessageList().find(req_id);

 qcril_qmi_radio_config_resp_data_type req_data;
 QCRIL_LOG_FUNC_ENTRY();
  do {
    if (pendingMsg == nullptr)
    {
      QCRIL_LOG_ERROR("pendingMsg is null");
      break;
    }
    if (transp_err != QMI_NO_ERR)
    {
      QCRIL_LOG_ERROR("Transp error (%d) recieved from QMI", transp_err);
      break;
    }
    req_data.data_len = resp_c_struct_len;
    req_data.data = resp_c_struct.get();
    if (pendingMsg->get_message_id() == QcRilRequestImsSetConfigMessage::get_class_message_id()) {
      auto msg(std::static_pointer_cast<QcRilRequestImsSetConfigMessage>(pendingMsg));
      qcril::interfaces::ConfigInfo &configInfo = msg->getConfigInfo();
      config_item =
        qcril_qmi_ims_map_ims_config_to_radio_config_item(configInfo.getItem());
      QCRIL_LOG_INFO("Calling config dispatch response helper, Config_item: %d", config_item);
      qcril_qmi_imss_dispatch_config_response_helper(&req_data, config_item, msg_id, req_id);
    }
    if (pendingMsg->get_message_id() == QcRilRequestImsGetConfigMessage::get_class_message_id())
    {
      auto msg(std::static_pointer_cast<QcRilRequestImsGetConfigMessage>(pendingMsg));
      qcril::interfaces::ConfigInfo &configInfo = msg->getConfigInfo();
      config_item =
         qcril_qmi_ims_map_ims_config_to_radio_config_item(configInfo.getItem());
      QCRIL_LOG_INFO("Calling config dispatch response helper, Config_item: %d", config_item);
      qcril_qmi_imss_dispatch_config_response_helper(&req_data, config_item, msg_id, req_id);
    }
  } while (FALSE);

  QCRIL_LOG_FUNC_RETURN();

}

//===========================================================================

 //===========================================================================
// qcril_qmi_imss_get_client_provisioning_config
//===========================================================================
void qcril_qmi_imss_get_client_provisioning_config()
{
   qmi_client_error_type qmi_client_error = QMI_NO_ERR;
   ims_settings_get_client_provisioning_config_rsp_msg_v01 qmi_ims_get_client_provisioning_config_resp;
   uint8_t wifi_call_valid = FALSE;
   ims_settings_wfc_status_enum_v01 wifi_call;
   uint8_t wifi_call_preference_valid = FALSE;
   ims_settings_wfc_preference_v01 wifi_call_preference;
   bool send_update_to_data = false;

   QCRIL_LOG_FUNC_ENTRY();

   qcril_qmi_imss_info_lock();
   wifi_call_valid = qcril_qmi_ims_cached_info.wifi_calling_enabled_valid;
   wifi_call = qcril_qmi_imss_convert_ril_to_imss_wfc_status(
           qcril_qmi_ims_cached_info.wifi_calling_enabled);
   wifi_call_preference_valid = qcril_qmi_ims_cached_info.call_mode_preference_valid;
   wifi_call_preference = qcril_qmi_imss_convert_ril_to_imss_wfc_preference(
           qcril_qmi_ims_cached_info.call_mode_preference);
   qcril_qmi_imss_info_unlock();

   if(wifi_call_preference_valid && wifi_call_valid)
   {
      QCRIL_LOG_INFO("wifi_call status %d, wifi_call_preference: %d", wifi_call, wifi_call_preference);
      return;
   }

   memset(&qmi_ims_get_client_provisioning_config_resp, 0x0,
       sizeof(qmi_ims_get_client_provisioning_config_resp));

   qmi_client_error = qmi_client_imss_send_sync(
       QMI_IMS_SETTINGS_GET_CLIENT_PROVISIONING_CONFIG_REQ_V01,
       NULL,
       0,
       &qmi_ims_get_client_provisioning_config_resp,
       sizeof(qmi_ims_get_client_provisioning_config_resp));
   QCRIL_LOG_INFO(".. qmi send sync res %d", (int) qmi_client_error );

   if (qmi_client_error == QMI_NO_ERR)
   {
      if( qmi_ims_get_client_provisioning_config_resp.resp.result == QMI_RESULT_SUCCESS_V01 )
      {
          qcril_qmi_imss_info_lock();
          qcril_qmi_ims_cached_info.wifi_calling_enabled_valid = qmi_ims_get_client_provisioning_config_resp.wifi_call_valid;
          qcril_ims_setting_wfc_status_type old_value_status = qcril_qmi_ims_cached_info.wifi_calling_enabled;
          qcril_qmi_ims_cached_info.wifi_calling_enabled = qcril_qmi_imss_convert_imss_to_ril_wfc_status(
            qmi_ims_get_client_provisioning_config_resp.wifi_call);
          if(old_value_status != qcril_qmi_ims_cached_info.wifi_calling_enabled)
          {
            send_update_to_data = true;
          }
          qcril_qmi_ims_cached_info.call_mode_preference_valid = qmi_ims_get_client_provisioning_config_resp.wifi_call_preference_valid;
          qcril_ims_setting_wfc_preference_type old_value_pref = qcril_qmi_ims_cached_info.call_mode_preference;
          qcril_qmi_ims_cached_info.call_mode_preference = qcril_qmi_imss_convert_imss_to_ril_wfc_preference(
                qmi_ims_get_client_provisioning_config_resp.wifi_call_preference);
          if(old_value_pref != qcril_qmi_ims_cached_info.call_mode_preference)
          {
            send_update_to_data = true;
          }

          QCRIL_LOG_INFO(".. client_prov_enabled_valid: %d, client_prov_enabled: %d",
            qmi_ims_get_client_provisioning_config_resp.enable_client_provisioning_valid,
            qmi_ims_get_client_provisioning_config_resp.enable_client_provisioning);
          QCRIL_LOG_INFO(".. wifi_call_valid: %d, wifi_call: %d", qcril_qmi_ims_cached_info.wifi_calling_enabled_valid,
            qcril_qmi_ims_cached_info.wifi_calling_enabled);
          QCRIL_LOG_INFO(".. wifi_call_preference_valid: %d, wifi_call_preference: %d", qcril_qmi_ims_cached_info.call_mode_preference_valid,
            qcril_qmi_ims_cached_info.call_mode_preference);

          qcril_qmi_imss_info_unlock();
      }
   }

   if (send_update_to_data)
   {
     qcril_qmi_imss_update_wifi_pref_from_ind_to_qcril_data();
   }

   /* IMSS REFACTOR: TEST */
   qcril_qmi_imss_broadcast_wfc_settings(qcril_qmi_ims_cached_info);

   QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imss_get_client_provisioning_config

//===========================================================================
// qcril_qmi_imss_client_provisioning_config_ind_hdlr
//===========================================================================
void qcril_qmi_imss_client_provisioning_config_ind_hdlr(void *ind_data_ptr)
{
  ims_settings_client_provisioning_config_ind_msg_v01 *reg_ind_msg_ptr =
      (ims_settings_client_provisioning_config_ind_msg_v01 *)ind_data_ptr;
  bool send_update_to_data = false;

  QCRIL_LOG_FUNC_ENTRY();

  if (NULL != reg_ind_msg_ptr)
  {
    qcril_qmi_imss_info_lock();
    if (reg_ind_msg_ptr->wifi_call_valid)
    {
      qcril_qmi_ims_cached_info.wifi_calling_enabled_valid = reg_ind_msg_ptr->wifi_call_valid;
      qcril_ims_setting_wfc_status_type old_value_status =
          qcril_qmi_ims_cached_info.wifi_calling_enabled;
      qcril_qmi_ims_cached_info.wifi_calling_enabled =
          qcril_qmi_imss_convert_imss_to_ril_wfc_status(reg_ind_msg_ptr->wifi_call);
      if (old_value_status != qcril_qmi_ims_cached_info.wifi_calling_enabled)
      {
        send_update_to_data = true;
      }
    }
    if (reg_ind_msg_ptr->wifi_call_preference_valid)
    {
      qcril_qmi_ims_cached_info.call_mode_preference_valid =
          reg_ind_msg_ptr->wifi_call_preference_valid;
      qcril_ims_setting_wfc_preference_type old_value_pref =
          qcril_qmi_ims_cached_info.call_mode_preference;
      qcril_qmi_ims_cached_info.call_mode_preference =
          qcril_qmi_imss_convert_imss_to_ril_wfc_preference(reg_ind_msg_ptr->wifi_call_preference);
      if (old_value_pref != qcril_qmi_ims_cached_info.call_mode_preference)
      {
        send_update_to_data = true;
      }
    }
    QCRIL_LOG_INFO(".. client_prov_enabled_valid: %d, client_prov_enabled: %d",
                   reg_ind_msg_ptr->client_provisioning_enabled_valid,
                   reg_ind_msg_ptr->client_provisioning_enabled);
    QCRIL_LOG_INFO(".. wifi_call_valid: %d, wifi_call: %d",
                   qcril_qmi_ims_cached_info.wifi_calling_enabled_valid,
                   qcril_qmi_ims_cached_info.wifi_calling_enabled);
    QCRIL_LOG_INFO(".. wifi_call_preference_valid: %d, wifi_call_preference: %d",
                   qcril_qmi_ims_cached_info.call_mode_preference_valid,
                   qcril_qmi_ims_cached_info.call_mode_preference);

    /* IMSA refactoring */
    qcril_qmi_imss_broadcast_wfc_settings(qcril_qmi_ims_cached_info);

    qcril_qmi_imss_info_unlock();

    if (send_update_to_data)
    {
      qcril_qmi_imss_update_wifi_pref_from_ind_to_qcril_data();
    }
  }
  else
  {
    QCRIL_LOG_ERROR("ind_data_ptr is NULL");
  }

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// qcril_qmi_imss_unsol_ind_cb_helper
//===========================================================================
void qcril_qmi_imss_unsol_ind_cb_helper
(
 unsigned int   msg_id,
 unsigned char *decoded_payload,
 uint32_t       decoded_payload_len
)
{
  QCRIL_LOG_FUNC_ENTRY();

  if (decoded_payload || !decoded_payload_len)
  {
    switch(msg_id)
    {
      case QMI_IMS_SETTINGS_CLIENT_PROVISIONING_CONFIG_IND_V01:
        qcril_qmi_imss_client_provisioning_config_ind_hdlr(decoded_payload);
        break;

      default:
        QCRIL_LOG_INFO("Unknown QMI IMSA indication %d", msg_id);
        break;
    }
  }

  QCRIL_LOG_FUNC_RETURN();
} // qcril_qmi_imss_unsol_ind_cb_helper

//===========================================================================
// qcril_qmi_imss_send_set_qipcall_config_req
//===========================================================================
RIL_Errno qcril_qmi_imss_send_set_qipcall_config_req
(
 const ims_settings_set_qipcall_config_req_msg_v01 *qmi_req,
 uint32 user_data
)
{
  qmi_client_error_type qmi_client_error = QMI_NO_ERR;
  RIL_Errno res = RIL_E_GENERIC_FAILURE;

  QCRIL_LOG_FUNC_ENTRY();

  qmi_client_error = qmi_client_imss_send_async(
      QMI_IMS_SETTINGS_SET_QIPCALL_CONFIG_REQ_V01,
      (void *)qmi_req,
      sizeof(ims_settings_set_qipcall_config_req_msg_v01),
      sizeof(ims_settings_set_qipcall_config_rsp_msg_v01),
      qcril_qmi_imss_set_qipcall_config_resp_hdlr,
      (void*)(uintptr_t)user_data);


  QCRIL_LOG_INFO(".. qmi send async res %d", (int) qmi_client_error);

  if (qmi_client_error == QMI_NO_ERR)
  {
    res = RIL_E_SUCCESS;
  }

  QCRIL_LOG_FUNC_RETURN();

  return res;
} // qcril_qmi_imss_send_set_qipcall_config_req


//===========================================================================
//  qcril_qmi_imss_convert_ril_to_imss_wfc_status
//===========================================================================
ims_settings_wfc_status_enum_v01 qcril_qmi_imss_convert_ril_to_imss_wfc_status
(
  qcril_ims_setting_wfc_status_type wfc_status
)
{
  ims_settings_wfc_status_enum_v01 res;

  switch(wfc_status)
  {
    case QCRIL_IMS_SETTING_WFC_NOT_SUPPORTED:
      res = IMS_SETTINGS_WFC_STATUS_NOT_SUPPORTED_V01;
      break;

    case QCRIL_IMS_SETTING_WFC_OFF:
      res = IMS_SETTINGS_WFC_STATUS_OFF_V01;
      break;

    case QCRIL_IMS_SETTING_WFC_ON:
      res = IMS_SETTINGS_WFC_STATUS_ON_V01;
      break;

    default:
      res = IMS_SETTINGS_WFC_STATUS_NOT_SUPPORTED_V01;
  }
  return res;
}

//===========================================================================
//  qcril_qmi_imss_convert_ril_to_imss_wfc_preference
//===========================================================================
ims_settings_wfc_preference_v01 qcril_qmi_imss_convert_ril_to_imss_wfc_preference
(
  qcril_ims_setting_wfc_preference_type wfc_preference
)
{
  ims_settings_wfc_preference_v01 res;

  switch(wfc_preference)
  {
    case QCRIL_IMS_SETTING_WFC_WLAN_PREFERRED:
      res = IMS_SETTINGS_WFC_WLAN_PREFERRED_V01;
      break;

    case QCRIL_IMS_SETTING_WFC_WLAN_ONLY:
      res = IMS_SETTINGS_WFC_WLAN_ONLY_V01;
      break;

    case QCRIL_IMS_SETTING_WFC_CELLULAR_PREFERRED:
      res = IMS_SETTINGS_WFC_CELLULAR_PREFERRED_V01;
      break;

    case QCRIL_IMS_SETTING_WFC_CELLULAR_ONLY:
      res = IMS_SETTINGS_WFC_CELLULAR_ONLY_V01;
      break;

    default:
      res = IMS_SETTINGS_WFC_CALL_PREF_NONE_V01;
  }
  return res;
}
#endif
