/******************************************************************************
#  Copyright (c) 2015, 2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/******************************************************************************
  @file    qcril_qmi_radio_config_dispatch_helper.c
  @brief   qcril qmi - radio config set/get function handlers

  DESCRIPTION
    Handles radio config set/get functions.
    Dispatches the config items by grouping them wrt
    their QMI messages, and calling the respective
    function handlers.

******************************************************************************/
#ifdef FEATURE_QCRIL_RADIO_CONFIG_SOCKET

#include "qcril_qmi_radio_config_dispatch_helper.h"
#include "qcril_qmi_radio_config_socket.h"
#include "qcril_memory_management.h"
#include "SetQualityMeasurementMessage.h"
#include <framework/GenericCallback.h>
#include <modules/android/RilRequestMessage.h>
#include <interfaces/lte_direct/QcRilRequestSetLtedCategoryMessage.h>
#include <interfaces/lte_direct/QcRilRequestSetLtedConfigMessage.h>
#include <interfaces/lte_direct/QcRilRequestGetLtedConfigMessage.h>
#include <interfaces/lte_direct/QcRilRequestGetLtedCategoryMessage.h>

#include "framework/Log.h"
extern "C" {
#include "data_system_determination_v01.h"
#include "qcril_qmi_radio_config_misc.h"
#include "qcril_qmi_radio_config_data.h"
#include "qcrili.h"
#include "qcril_qmi_radio_config_packing.h"
#include "radio_config_interface.pb.h"
#include "common_v01.h"
}

#define TAG "QCRIL_RADIO_CONFIG"

class RadioConfigDataSetQualityMeasurementCallback : public GenericCallback<qmi_response_type_v01> {
private:
    legacy_request_payload params;
    com_qualcomm_qti_radioconfiginterface_ConfigMsg radio_config_resp;

public:
    inline RadioConfigDataSetQualityMeasurementCallback(string clientToken,
        const qcril_request_params_type &p, com_qualcomm_qti_radioconfiginterface_ConfigMsg &resp_data) :
                GenericCallback(clientToken), params(p)
    {
        radio_config_resp.settings_count = resp_data.settings_count;
        size_t settings_size = sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg) * radio_config_resp.settings_count;
        radio_config_resp.settings = (com_qualcomm_qti_radioconfiginterface_ConfigItemMsg*)malloc(settings_size);
        if (radio_config_resp.settings != NULL && resp_data.settings != NULL)
        {
            memcpy(radio_config_resp.settings, resp_data.settings, settings_size);
        }
        else
        {
            radio_config_resp.settings = NULL;
        }
    }

    inline ~RadioConfigDataSetQualityMeasurementCallback()
    {
        if (radio_config_resp.settings != NULL)
        {
            free(radio_config_resp.settings);
        }
    }

    Message::Callback *clone() override;

    void onResponse(std::shared_ptr<Message> solicitedMsg, Status status,
        std::shared_ptr<qmi_response_type_v01> responseDataPtr) override;

    qcril_request_params_type &get_params() {
        return params.get_params();
    }
};

void RadioConfigDataSetQualityMeasurementCallback::onResponse(
    std::shared_ptr<Message> solicitedMsg, Status status,
    std::shared_ptr<qmi_response_type_v01> responseDataPtr) {

    boolean fill_send_err_resp = FALSE;
    size_t i = 0;
    if (status == Message::Callback::Status::SUCCESS && radio_config_resp.settings != NULL && responseDataPtr != NULL) {
        Log::getInstance().d("[RadioConfigDataSetQualityMeasurementCallback]: Callback[msg = " +
            solicitedMsg->dump() + "] executed. client data = " +
            mClientToken + " status = Message::Callback::Status::SUCCESS");
        if (responseDataPtr->result != QMI_RESULT_SUCCESS_V01)
        {
          fill_send_err_resp = TRUE;
        }
        else
        {
          qcril_qmi_radio_config_socket_send((RIL_Token)params.get_params().t,
              com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
              com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG,
              0, com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
              (void*)&radio_config_resp, sizeof(radio_config_resp));
        }
    }
    else if(radio_config_resp.settings != NULL){
      fill_send_err_resp = TRUE;
    }
    else {
        Log::getInstance().d("[RadioConfigDataSetQualityMeasurementCallback]: Callback[msg = " +
            solicitedMsg->dump() + "] executed. client data = " +
            mClientToken + " FAILURE !!");
        qcril_qmi_radio_config_socket_send((RIL_Token)params.get_params().t,
            com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
            com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG,
            0, com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_GENERIC_FAILURE,
            NULL,0);
   }
   if(fill_send_err_resp) {
     //if not success fill in the data structure with error
     for (i = 0; i < radio_config_resp.settings_count; i++)
     {
       if (radio_config_resp.settings[i].errorCause == com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR)
       {
         radio_config_resp.settings[i].errorCause = com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
       }
     }
     qcril_qmi_radio_config_socket_send((RIL_Token)params.get_params().t,
         com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
         com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG,
         0, com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
         (void*)&radio_config_resp, sizeof(radio_config_resp));

   }
}

Message::Callback *RadioConfigDataSetQualityMeasurementCallback::clone() {
    RadioConfigDataSetQualityMeasurementCallback *clone =
                     new RadioConfigDataSetQualityMeasurementCallback(mClientToken, params.get_params(), radio_config_resp);
    return clone;
}

typedef struct RadioConfigReqParams_t {
  RIL_Token token;
  com_qualcomm_qti_radioconfiginterface_ConfigItem item;
  RadioConfigReqParams_t(RIL_Token t, com_qualcomm_qti_radioconfiginterface_ConfigItem i)
      : token(t), item(i) {}
} RadioConfigReqParams;

/*=========================================================================
FUNCTION:  qcril_qmi_lte_direct_disc_dispatcher_req_handler
===========================================================================*/
/*!
  @brief
  Lte direct discovery dispatch handler for provisioning requests

  @return
  RIL_Errno
 */
/*=========================================================================*/
uint8_t qcril_qmi_lte_direct_disc_dispatcher_req_handler(
    RIL_Token token, com_qualcomm_qti_radioconfiginterface_ConfigMsg *config_msg_ptr,
    com_qualcomm_qti_radioconfiginterface_ConfigMsg *resp_ptr /* Out parameter */
) {
  com_qualcomm_qti_radioconfiginterface_ConfigFailureCause error_cause;
  com_qualcomm_qti_radioconfiginterface_ConfigItemMsg temp;
  com_qualcomm_qti_radioconfiginterface_LtedConfig *config = NULL;
  pb_bytes_array_t *byte_array_value = NULL;
  int item;
  uint8_t ret = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  if (config_msg_ptr->settings_count == 1) {
    temp = config_msg_ptr->settings[0];
    item = qcril_qmi_radio_config_map_socket_item_to_config_item(temp.item);

    error_cause = com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;

    switch (item) {
    /* SET requests */
    case QCRIL_QMI_RADIO_CONFIG_SET_LTED_CONFIG:
      QCRIL_LOG_ERROR("QCRIL_QMI_RADIO_CONFIG_SET_LTED_CONFIG");
      if (temp.byteArrayValue != NULL) {
        QCRIL_LOG_ERROR("temp.byteArrayValue->size = %d", temp.byteArrayValue->size);
        if (temp.byteArrayValue->size > 0) {
          config = qcril_qmi_radio_config_unpack_lted_config(temp.byteArrayValue->bytes,
                                                             temp.byteArrayValue->size);

          QCRIL_LOG_ERROR("config = 0x%x", config);
          if (config != NULL) {
            QCRIL_LOG_ERROR("config->apn = %s", config->apn);
            QCRIL_LOG_ERROR("config->announcingPolicy_count = 0x%x",
                            config->announcingPolicy_count);
            QCRIL_LOG_ERROR("config->monitoringPolicy_count = 0x%x",
                            config->monitoringPolicy_count);
            auto msg = std::make_shared<QcRilRequestSetLtedConfigMessage>(
                std::make_shared<MessageContext>(QCRIL_DEFAULT_INSTANCE_ID));
            if (msg) {
              if (config->has_osId) {
                qcril::interfaces::lte_direct::Value128 osId = {};
                if (config->osId.has_lsb) {
                  osId.setLsb(config->osId.lsb);
                }
                if (config->osId.has_msb) {
                  osId.setMsb(config->osId.msb);
                }
                msg->setOsId(osId);
              }
              if (config->apn) {
                msg->setApn(config->apn);
              }

              if (config->announcingPolicy_count > 0) {
                std::vector<qcril::interfaces::lte_direct::AnnouncingPolicy> annPolicyVec;
                for (size_t i = 0; i < config->announcingPolicy_count; i++) {
                  com_qualcomm_qti_radioconfiginterface_AnnouncingPolicy ann_policy =
                      config->announcingPolicy[i];
                  qcril::interfaces::lte_direct::AnnouncingPolicy tempAnnPolicy = {};

                  if (ann_policy.plmn) {
                    auto plmn = std::make_shared<qcril::interfaces::lte_direct::Plmn>();
                    if (plmn) {
                      if (ann_policy.plmn->mcc) {
                        plmn->setMcc(ann_policy.plmn->mcc);
                      }
                      if (ann_policy.plmn->mnc) {
                        plmn->setMnc(ann_policy.plmn->mnc);
                      }
                    }
                    tempAnnPolicy.setPlmn(plmn);
                  }

                  if (ann_policy.has_validityTime) {
                    // T4005 ValidityTime
                    tempAnnPolicy.setValidityTime(ann_policy.validityTime);
                  }

                  if (ann_policy.has_range) {
                    // Range
                    switch (ann_policy.range) {
                    case 0:
                      tempAnnPolicy.setRange(qcril::interfaces::lte_direct::Range::INVALID);
                      break;
                    case 1:
                      tempAnnPolicy.setRange(qcril::interfaces::lte_direct::Range::SHORT);
                      break;
                    case 2:
                      tempAnnPolicy.setRange(qcril::interfaces::lte_direct::Range::MEDIUM);
                      break;
                    case 3:
                      tempAnnPolicy.setRange(qcril::interfaces::lte_direct::Range::LONG);
                      break;
                    case 4:
                      tempAnnPolicy.setRange(qcril::interfaces::lte_direct::Range::RESERVED);
                      break;
                    default:
                      break;
                    }
                  }
                  annPolicyVec.push_back(tempAnnPolicy);
                }
                msg->setAnnouncingPolicy(annPolicyVec);
              }

              if (config->monitoringPolicy_count > 0) {
                std::vector<qcril::interfaces::lte_direct::MonitoringPolicy> monPolicyVec;
                for (size_t i = 0; i < config->monitoringPolicy_count; i++) {
                  com_qualcomm_qti_radioconfiginterface_MonitoringPolicy mon_policy =
                      config->monitoringPolicy[i];
                  qcril::interfaces::lte_direct::MonitoringPolicy tempMonPolicy = {};

                  // PLMN
                  if (mon_policy.plmn) {
                    auto plmn = std::make_shared<qcril::interfaces::lte_direct::Plmn>();
                    if (plmn) {
                      if (mon_policy.plmn->mcc) {
                        plmn->setMcc(mon_policy.plmn->mcc);
                      }
                      if (mon_policy.plmn->mnc) {
                        plmn->setMnc(mon_policy.plmn->mnc);
                      }
                    }
                    tempMonPolicy.setPlmn(plmn);
                  }

                  if (mon_policy.has_validityTime) {
                    // T4005 ValidityTime
                    tempMonPolicy.setValidityTime(mon_policy.validityTime);
                  }

                  if (mon_policy.has_remainingTime) {
                    // RemainingTime
                    tempMonPolicy.setRemainingTime(mon_policy.remainingTime);
                  }
                  monPolicyVec.push_back(tempMonPolicy);
                }
                msg->setMonitoringPolicy(monPolicyVec);
              }
              QCRIL_LOG_INFO("config->bakPassword->size = %d",
                             config->bakPassword ? config->bakPassword->size : -1);
              if (config->bakPassword && config->bakPassword->size > 0) {
                std::vector<uint8_t> bakPassword;
                bakPassword.resize(config->bakPassword->size);
                for (size_t i = 0; i < config->bakPassword->size; i++) {
                  bakPassword[i] = config->bakPassword->bytes[i];
                }
                msg->setBakPassword(bakPassword);
              }
              std::shared_ptr<void> userData =
                  std::make_shared<RadioConfigReqParams>(token, temp.item);
              msg->setUserData(userData);

              GenericCallback<QcRilRequestMessageCallbackPayload>
              cb([](std::shared_ptr<Message> msg, Message::Callback::Status status,
                    std::shared_ptr<QcRilRequestMessageCallbackPayload> resp) -> void {
                uint32_t errorCode = RIL_E_GENERIC_FAILURE;
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause error_cause =
                    com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
                if (status == Message::Callback::Status::SUCCESS && resp != nullptr) {
                  errorCode = resp->errorCode;
                  if (errorCode == RIL_E_SUCCESS) {
                    error_cause =
                        com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
                  }
                }
                if (msg) {
                  auto reqMsg(std::static_pointer_cast<QcRilRequestSetLtedConfigMessage>(msg));
                  if (reqMsg->hasUserData() && reqMsg->getUserData()) {
                    auto userData(
                        std::static_pointer_cast<RadioConfigReqParams>(reqMsg->getUserData()));
                    com_qualcomm_qti_radioconfiginterface_ConfigMsg radio_config_resp = {};
                    radio_config_resp.settings_count = 1;
                    radio_config_resp.settings =
                        (com_qualcomm_qti_radioconfiginterface_ConfigItemMsg *)malloc(
                            sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg));
                    radio_config_resp.settings[0].item = userData->item;
                    radio_config_resp.settings[0].has_errorCause = TRUE;
                    radio_config_resp.settings[0].errorCause = error_cause;

                    qcril_qmi_radio_config_socket_send(
                        userData->token,
                        com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
                        com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG, 0,
                        com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
                        (void *)&radio_config_resp, sizeof(radio_config_resp));
                  }
                }
              });
              msg->setCallback(&cb);
              msg->dispatch();
              error_cause = com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
            }
          }
        } else {
          QCRIL_LOG_ERROR("Invalid data: temp.byteArrayValue->size is 0");
        }
      } else {
        QCRIL_LOG_ERROR("Invalid data: temp.byteArrayValue is NULL");
      }
      ret = TRUE;
      break;

    case QCRIL_QMI_RADIO_CONFIG_SET_LTED_CATEGORY:
      QCRIL_LOG_ERROR("QCRIL_QMI_RADIO_CONFIG_SET_LTED_CATEGORY");
      if (temp.byteArrayValue != NULL) {
        QCRIL_LOG_ERROR("temp.byteArrayValue->size = %d", temp.byteArrayValue->size);
        if (temp.byteArrayValue->size > 0) {
          com_qualcomm_qti_radioconfiginterface_LtedCategory *category = NULL;

          category = qcril_qmi_radio_config_unpack_lted_category(temp.byteArrayValue->bytes,
                                                                 temp.byteArrayValue->size);

          QCRIL_LOG_ERROR("category = 0x%x", category);
          if (category != NULL) {
            QCRIL_LOG_ERROR("category->osAppId = %s", category->osAppId);
            QCRIL_LOG_ERROR("category->has_category = %d, category->category = %d",
                            category->has_category, category->category);

            auto msg = std::make_shared<QcRilRequestSetLtedCategoryMessage>(
                std::make_shared<MessageContext>(QCRIL_DEFAULT_INSTANCE_ID));
            if (msg) {
              if (category->osAppId) {
                msg->setOsAppId(category->osAppId);
              }
              if (category->has_category) {
                switch (category->category) {
                case 0:
                  msg->setCategory(qcril::interfaces::lte_direct::Category::HIGH);
                  break;
                case 1:
                  msg->setCategory(qcril::interfaces::lte_direct::Category::MEDIUM);
                  break;
                case 2:
                  msg->setCategory(qcril::interfaces::lte_direct::Category::LOW);
                  break;
                case 3:
                  msg->setCategory(qcril::interfaces::lte_direct::Category::VERY_LOW);
                  break;
                case 4:
                  msg->setCategory(qcril::interfaces::lte_direct::Category::INVALID);
                  break;
                default:
                  break;
                }
              }
              std::shared_ptr<void> userData =
                  std::make_shared<RadioConfigReqParams>(token, temp.item);
              msg->setUserData(userData);

              GenericCallback<QcRilRequestMessageCallbackPayload>
              cb([](std::shared_ptr<Message> msg, Message::Callback::Status status,
                    std::shared_ptr<QcRilRequestMessageCallbackPayload> resp) -> void {
                uint32_t errorCode = RIL_E_GENERIC_FAILURE;
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause error_cause =
                    com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
                if (status == Message::Callback::Status::SUCCESS && resp != nullptr) {
                  errorCode = resp->errorCode;
                  if (errorCode == RIL_E_SUCCESS) {
                    error_cause =
                        com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
                  }
                }
                if (msg) {
                  auto reqMsg(std::static_pointer_cast<QcRilRequestSetLtedCategoryMessage>(msg));
                  if (reqMsg->hasUserData() && reqMsg->getUserData()) {
                    auto userData(
                        std::static_pointer_cast<RadioConfigReqParams>(reqMsg->getUserData()));
                    com_qualcomm_qti_radioconfiginterface_ConfigMsg radio_config_resp = {};
                    radio_config_resp.settings_count = 1;
                    radio_config_resp.settings =
                        (com_qualcomm_qti_radioconfiginterface_ConfigItemMsg *)malloc(
                            sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg));
                    radio_config_resp.settings[0].item = userData->item;
                    radio_config_resp.settings[0].has_errorCause = TRUE;
                    radio_config_resp.settings[0].errorCause = error_cause;

                    qcril_qmi_radio_config_socket_send(
                        userData->token,
                        com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
                        com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG, 0,
                        com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
                        (void *)&radio_config_resp, sizeof(radio_config_resp));
                  }
                }
              });
              msg->setCallback(&cb);
              msg->dispatch();
              error_cause = com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
            }
          }
        } else {
          QCRIL_LOG_ERROR("Invalid data: temp.byteArrayValue->size is 0");
        }
      } else {
        QCRIL_LOG_ERROR("Invalid data: temp.byteArrayValue is NULL");
      }
      ret = TRUE;
      break;

    /* GET Requests */
    case QCRIL_QMI_RADIO_CONFIG_GET_LTED_CONFIG:
      QCRIL_LOG_ERROR("QCRIL_QMI_RADIO_CONFIG_GET_LTED_CONFIG");
      {
        auto msg = std::make_shared<QcRilRequestGetLtedConfigMessage>(
            std::make_shared<MessageContext>(QCRIL_DEFAULT_INSTANCE_ID));
        if (msg) {
          std::shared_ptr<void> userData = std::make_shared<RadioConfigReqParams>(token, temp.item);
          msg->setUserData(userData);

          GenericCallback<QcRilRequestMessageCallbackPayload> cb(
              [](std::shared_ptr<Message> msg, Message::Callback::Status status,
                 std::shared_ptr<QcRilRequestMessageCallbackPayload> resp) -> void {
                uint32_t errorCode = RIL_E_GENERIC_FAILURE;
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause error_cause =
                    com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
                std::shared_ptr<qcril::interfaces::lte_direct::GetLtedConfigResp> respData =
                    nullptr;
                if (status == Message::Callback::Status::SUCCESS && resp != nullptr) {
                  errorCode = resp->errorCode;
                  if (errorCode == RIL_E_SUCCESS) {
                    respData =
                        std::static_pointer_cast<qcril::interfaces::lte_direct::GetLtedConfigResp>(
                            resp->data);
                  }
                }
                com_qualcomm_qti_radioconfiginterface_LtedConfig config = {};
                pb_bytes_array_t *byte_array_value = nullptr;
                if (respData) {
                  if (respData->hasApn() && !respData->getApn().empty()) {
                    config.apn = strdup(respData->getApn().c_str());
                  }
                  if (!respData->getAnnouncingPolicy().empty()) {
                    std::vector<qcril::interfaces::lte_direct::AnnouncingPolicy> policyList =
                        respData->getAnnouncingPolicy();
                    config.announcingPolicy_count = policyList.size();
                    config.announcingPolicy =
                        qcril_malloc2(config.announcingPolicy, config.announcingPolicy_count);
                    if (config.announcingPolicy) {
                      for (size_t i = 0; i < config.announcingPolicy_count; i++) {
                        qcril::interfaces::lte_direct::AnnouncingPolicy &tempPol = policyList[i];
                        config.announcingPolicy[i].plmn =
                            qcril_malloc2(config.announcingPolicy[i].plmn);
                        if (config.announcingPolicy[i].plmn && tempPol.hasPlmn()) {
                          auto plmn = tempPol.getPlmn();
                          if (plmn && plmn->hasMcc() && plmn->hasMnc()) {
                            config.announcingPolicy[i].plmn->mcc = strdup(plmn->getMcc().c_str());
                            config.announcingPolicy[i].plmn->mnc = strdup(plmn->getMnc().c_str());
                          }
                        }
                        if (tempPol.hasValidityTime()) {
                          config.announcingPolicy[i].has_validityTime = TRUE;
                          config.announcingPolicy[i].validityTime = tempPol.getValidityTime();
                        }
                        if (tempPol.hasRange()) {
                          config.announcingPolicy[i].has_range = TRUE;
                          switch (tempPol.getRange()) {
                          case qcril::interfaces::lte_direct::Range::INVALID:
                            config.announcingPolicy[i].range = 0;
                            break;
                          case qcril::interfaces::lte_direct::Range::SHORT:
                            config.announcingPolicy[i].range = 1;
                            break;
                          case qcril::interfaces::lte_direct::Range::MEDIUM:
                            config.announcingPolicy[i].range = 2;
                            break;
                          case qcril::interfaces::lte_direct::Range::LONG:
                            config.announcingPolicy[i].range = 3;
                            break;
                          case qcril::interfaces::lte_direct::Range::RESERVED:
                            config.announcingPolicy[i].range = 4;
                            break;
                          default:
                            config.announcingPolicy[i].has_range = FALSE;
                            break;
                          }
                        }
                      }
                    }
                  }

                  if (!respData->getMonitoringPolicy().empty()) {
                    std::vector<qcril::interfaces::lte_direct::MonitoringPolicy> policyList =
                        respData->getMonitoringPolicy();
                    config.monitoringPolicy_count = policyList.size();
                    config.monitoringPolicy =
                        qcril_malloc2(config.monitoringPolicy, config.monitoringPolicy_count);
                    if (config.monitoringPolicy) {
                      for (size_t i = 0; i < config.monitoringPolicy_count; i++) {
                        qcril::interfaces::lte_direct::MonitoringPolicy &tempPol = policyList[i];
                        config.monitoringPolicy[i].plmn =
                            qcril_malloc2(config.monitoringPolicy[i].plmn);
                        if (config.monitoringPolicy[i].plmn && tempPol.hasPlmn()) {
                          auto plmn = tempPol.getPlmn();
                          if (plmn && plmn->hasMcc() && plmn->hasMnc()) {
                            config.monitoringPolicy[i].plmn->mcc = strdup(plmn->getMcc().c_str());
                            config.monitoringPolicy[i].plmn->mnc = strdup(plmn->getMnc().c_str());
                          }
                        }
                        if (tempPol.hasValidityTime()) {
                          config.monitoringPolicy[i].has_validityTime = TRUE;
                          config.monitoringPolicy[i].validityTime = tempPol.getValidityTime();
                        }
                        if (tempPol.hasRemainingTime()) {
                          config.monitoringPolicy[i].has_remainingTime = TRUE;
                          config.monitoringPolicy[i].remainingTime = tempPol.getRemainingTime();
                        }
                      }
                    }
                  }
                  byte_array_value = qcril_qmi_radio_config_pack_lted_config(&config);
                  if (byte_array_value) {
                    error_cause =
                        com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
                  }
                }
                if (msg) {
                  auto reqMsg(std::static_pointer_cast<QcRilRequestGetLtedConfigMessage>(msg));
                  if (reqMsg->hasUserData() && reqMsg->getUserData()) {
                    auto userData(
                        std::static_pointer_cast<RadioConfigReqParams>(reqMsg->getUserData()));
                    com_qualcomm_qti_radioconfiginterface_ConfigMsg radio_config_resp = {};
                    radio_config_resp.settings_count = 1;
                    radio_config_resp.settings =
                        (com_qualcomm_qti_radioconfiginterface_ConfigItemMsg *)malloc(
                            sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg));
                    radio_config_resp.settings[0].item = userData->item;
                    radio_config_resp.settings[0].has_errorCause = TRUE;
                    radio_config_resp.settings[0].errorCause = error_cause;
                    radio_config_resp.settings[0].byteArrayValue = byte_array_value;

                    qcril_qmi_radio_config_socket_send(
                        userData->token,
                        com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
                        com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG, 0,
                        com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
                        (void *)&radio_config_resp, sizeof(radio_config_resp));
                  }
                }
              });
          msg->setCallback(&cb);
          msg->dispatch();
          error_cause = com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
        }
        ret = TRUE;
      }
      break;

    case QCRIL_QMI_RADIO_CONFIG_GET_LTED_CATEGORY:
      QCRIL_LOG_ERROR("QCRIL_QMI_RADIO_CONFIG_GET_LTED_CATEGORY");
      if (temp.byteArrayValue != NULL) {
        QCRIL_LOG_ERROR("temp.byteArrayValue->size = %d", temp.byteArrayValue->size);
        if (temp.byteArrayValue->size > 0) {
          com_qualcomm_qti_radioconfiginterface_LtedCategory *req_category = NULL;

          req_category = qcril_qmi_radio_config_unpack_lted_category(temp.byteArrayValue->bytes,
                                                                     temp.byteArrayValue->size);

          QCRIL_LOG_ERROR("req_category = 0x%x", req_category);
          if (req_category != NULL) {
            QCRIL_LOG_ERROR("req_category->osAppId = %s", req_category->osAppId);

            auto msg = std::make_shared<QcRilRequestGetLtedCategoryMessage>(
                std::make_shared<MessageContext>(QCRIL_DEFAULT_INSTANCE_ID));
            if (msg) {
              if (req_category->osAppId) {
                msg->setOsAppId(req_category->osAppId);
              }
              std::shared_ptr<void> userData =
                  std::make_shared<RadioConfigReqParams>(token, temp.item);
              msg->setUserData(userData);

              GenericCallback<QcRilRequestMessageCallbackPayload>
              cb([](std::shared_ptr<Message> msg, Message::Callback::Status status,
                    std::shared_ptr<QcRilRequestMessageCallbackPayload> resp) -> void {
                uint32_t errorCode = RIL_E_GENERIC_FAILURE;
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause error_cause =
                    com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
                std::shared_ptr<qcril::interfaces::lte_direct::GetLtedCategoryResp> respData =
                    nullptr;
                if (status == Message::Callback::Status::SUCCESS && resp != nullptr) {
                  errorCode = resp->errorCode;
                  if (errorCode == RIL_E_SUCCESS) {
                    respData = std::static_pointer_cast<
                        qcril::interfaces::lte_direct::GetLtedCategoryResp>(resp->data);
                  }
                }
                com_qualcomm_qti_radioconfiginterface_LtedCategory respCategory = {};
                pb_bytes_array_t *byte_array_value = nullptr;
                if (respData) {
                  if (respData->hasOsAppId() && !respData->getOsAppId().empty()) {
                    respCategory.osAppId = strdup(respData->getOsAppId().c_str());
                  }
                  if (respData->hasCategory()) {
                    respCategory.has_category = TRUE;
                    switch (respData->getCategory()) {
                    case qcril::interfaces::lte_direct::Category::HIGH:
                      respCategory.category = 0;
                      break;
                    case qcril::interfaces::lte_direct::Category::MEDIUM:
                      respCategory.category = 1;
                      break;
                    case qcril::interfaces::lte_direct::Category::LOW:
                      respCategory.category = 2;
                      break;
                    case qcril::interfaces::lte_direct::Category::VERY_LOW:
                      respCategory.category = 3;
                      break;
                    case qcril::interfaces::lte_direct::Category::INVALID:
                      respCategory.category = 4;
                      break;
                    default:
                      respCategory.has_category = FALSE;
                      break;
                    }
                  }

                  byte_array_value = qcril_qmi_radio_config_pack_lted_category(&respCategory);
                  if (byte_array_value) {
                    error_cause =
                        com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
                  }
                }
                if (msg) {
                  auto reqMsg(std::static_pointer_cast<QcRilRequestGetLtedCategoryMessage>(msg));
                  if (reqMsg->hasUserData() && reqMsg->getUserData()) {
                    auto userData(
                        std::static_pointer_cast<RadioConfigReqParams>(reqMsg->getUserData()));
                    com_qualcomm_qti_radioconfiginterface_ConfigMsg radio_config_resp = {};
                    radio_config_resp.settings_count = 1;
                    radio_config_resp.settings =
                        (com_qualcomm_qti_radioconfiginterface_ConfigItemMsg *)malloc(
                            sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg));
                    radio_config_resp.settings[0].item = userData->item;
                    radio_config_resp.settings[0].has_errorCause = TRUE;
                    radio_config_resp.settings[0].errorCause = error_cause;
                    radio_config_resp.settings[0].byteArrayValue = byte_array_value;

                    qcril_qmi_radio_config_socket_send(
                        userData->token,
                        com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
                        com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG, 0,
                        com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
                        (void *)&radio_config_resp, sizeof(radio_config_resp));
                  }
                }
              });
              msg->setCallback(&cb);
              msg->dispatch();
              error_cause = com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;
            }
          }
        } else {
          QCRIL_LOG_ERROR("Invalid data: temp.byteArrayValue->size is 0");
        }
      } else {
        QCRIL_LOG_ERROR("Invalid data: temp.byteArrayValue is NULL");
      }
      ret = TRUE;
      break;

    default:
      break;
    }

    if (ret == TRUE &&
        error_cause != com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR) {
      memset(&(resp_ptr->settings[0]), 0,
             sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg));
      resp_ptr->settings[0].item = temp.item;
      resp_ptr->settings[0].has_errorCause = TRUE;
      resp_ptr->settings[0].errorCause = error_cause;
      if (byte_array_value != NULL) {
        resp_ptr->settings[0].byteArrayValue = byte_array_value;
      }
    }
  }

  QCRIL_LOG_FUNC_RETURN();
  return ret;
}

/**
 * Analyzes and processes the set_config request data.
 */
void qcril_qmi_radio_config_dispatch_set_request
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr /*!< Output parameter */
)
{
  dsd_set_quality_measurement_info_req_msg_v01 data_set_quality_req;
  memset(&data_set_quality_req, 0, sizeof(data_set_quality_req));

  com_qualcomm_qti_radioconfiginterface_ConfigMsg resp_data;
  memset(&resp_data, 0, sizeof(com_qualcomm_qti_radioconfiginterface_ConfigMsg));

  uint8_t data_filled = FALSE;
  size_t msg_len = 0;

  QCRIL_LOG_FUNC_ENTRY();
  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("Invalid arguments, cannot process further..");
    return;
  }

  do {
    com_qualcomm_qti_radioconfiginterface_ConfigMsg* config_msg_ptr =
      (com_qualcomm_qti_radioconfiginterface_ConfigMsg*)(params_ptr->data);
    if(config_msg_ptr == NULL)
    {
      //To return NULL RESPONSE?? or to handle such case in Java layer
      //could be that anotgher client(not our java service) might send it this way.
      //HOW TO HANDLE THIS?
      QCRIL_LOG_ERROR("No config items in the message!");
      break; //or send empty response with error?
    }

    QCRIL_LOG_INFO("SET_RADIO_CONFIG_REQ with %d config items.", config_msg_ptr->settings_count);

    //HARDCODED, NEEDS TO CHANGE WHILE MERGING.
    //THIS IS FOR TEST PURPOSE, BEFORE THE FINAL CHANGE IS READY.
    size_t i;
    com_qualcomm_qti_radioconfiginterface_ConfigItemMsg temp;
    qcril_qmi_radio_config_item_type item;

    //response ptr
    resp_data.settings_count = config_msg_ptr->settings_count;
    resp_data.settings = (com_qualcomm_qti_radioconfiginterface_ConfigItemMsg*)qcril_malloc(sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg) * config_msg_ptr->settings_count);
    if(resp_data.settings == NULL)
    {
      resp_data.settings_count = 0;
      QCRIL_LOG_ERROR("malloc failed");
      break;
    }
    msg_len = sizeof(resp_data);

    if (qcril_qmi_lte_direct_disc_dispatcher_req_handler(params_ptr->t, config_msg_ptr, &resp_data))
    {
      QCRIL_LOG_INFO("Handled in qcril_qmi_lte_direct_disc_dispatcher; completed!!!");
      data_filled = TRUE;
      break;
    }

    for(i = 0; i < config_msg_ptr->settings_count; i++)
    {
      temp = config_msg_ptr->settings[i];
      item = qcril_qmi_radio_config_map_socket_item_to_config_item(temp.item);

      resp_data.settings[i].item = temp.item;
      resp_data.settings[i].has_errorCause = TRUE;
      resp_data.settings[i].errorCause =
        com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;

      QCRIL_LOG_INFO("item id: %d to be set to: %d", item, temp.intValue);

      switch(item)
      {
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IN_CALL_LTE_RSRP_LOW:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.in_call_lte_rsrp_low_valid = TRUE;
              data_set_quality_req.in_call_lte_rsrp_low = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IN_CALL_LTE_RSRP_MID:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
              {
                data_filled = TRUE;
                data_set_quality_req.in_call_lte_rsrp_mid_valid = TRUE;
                data_set_quality_req.in_call_lte_rsrp_mid = (uint16_t)(temp.intValue);
              }
              else
              {
                QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
                resp_data.settings[i].errorCause =
                  com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
              }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IN_CALL_LTE_RSRP_HIGH:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.in_call_lte_rsrp_high_valid = TRUE;
              data_set_quality_req.in_call_lte_rsrp_high = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IN_CALL_WIFI_RSSI_THRESHOLD_LOW:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
              {
                data_filled = TRUE;
                data_set_quality_req.in_call_wifi_rssi_threshold_low_valid = TRUE;
                data_set_quality_req.in_call_wifi_rssi_threshold_low = (uint16_t)(temp.intValue);
              }
              else
              {
                QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
                resp_data.settings[i].errorCause =
                  com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
              }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IN_CALL_WIFI_RSSI_THRESHOLD_HIGH:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.in_call_wifi_rssi_threshold_high_valid = TRUE;
              data_set_quality_req.in_call_wifi_rssi_threshold_high = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IN_CALL_WIFI_SINR_THRESHOLD_LOW:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX ))
            {
              data_filled = TRUE;
              data_set_quality_req.in_call_wifi_sinr_threshold_low_valid = TRUE;
              data_set_quality_req.in_call_wifi_sinr_threshold_low = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IN_CALL_WIFI_SINR_THRESHOLD_HIGH:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.in_call_wifi_sinr_threshold_high_valid = TRUE;
              data_set_quality_req.in_call_wifi_sinr_threshold_high = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IDLE_LTE_RSRP_LOW:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.idle_lte_rsrp_low_valid = TRUE;
              data_set_quality_req.idle_lte_rsrp_low = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IDLE_LTE_RSRP_MID:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.idle_lte_rsrp_mid_valid = TRUE;
              data_set_quality_req.idle_lte_rsrp_mid = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IDLE_LTE_RSRP_HIGH:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.idle_lte_rsrp_high_valid = TRUE;
              data_set_quality_req.idle_lte_rsrp_high = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IDLE_WIFI_RSSI_THRESHOLD_LOW:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.idle_wifi_rssi_threshold_low_valid = TRUE;
              data_set_quality_req.idle_wifi_rssi_threshold_low = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IDLE_WIFI_RSSI_THRESHOLD_HIGH:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.idle_wifi_rssi_threshold_high_valid = TRUE;
              data_set_quality_req.idle_wifi_rssi_threshold_high = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IDLE_WIFI_SINR_THRESHOLD_LOW:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.idle_wifi_sinr_threshold_low_valid = TRUE;
              data_set_quality_req.idle_wifi_sinr_threshold_low = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_IDLE_WIFI_SINR_THRESHOLD_HIGH:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.idle_wifi_sinr_threshold_high_valid = TRUE;
              data_set_quality_req.idle_wifi_sinr_threshold_high = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_ECIO_1X_THRESHOLD_LOW:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.ecio_1x_threshold_low_valid = TRUE;
              data_set_quality_req.ecio_1x_threshold_low = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        case QCRIL_QMI_RADIO_CONFIG_DATA_QUALITY_MEASUREMENT_ECIO_1X_THRESHOLD_HIGH:
          if(temp.has_intValue)
          {
            if( (SHRT_MIN <= (int)temp.intValue) && ((int)temp.intValue <= SHRT_MAX) )
            {
              data_filled = TRUE;
              data_set_quality_req.ecio_1x_threshold_high_valid = TRUE;
              data_set_quality_req.ecio_1x_threshold_high = (uint16_t)(temp.intValue);
            }
            else
            {
              QCRIL_LOG_ERROR("item value is greater than the expected maximum!");
              resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
            }
          }
          else
          {
            QCRIL_LOG_ERROR("item doesnt have int value");
            resp_data.settings[i].errorCause =
              com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_GENERIC_ERR;
          }
          break;
        default:
          QCRIL_LOG_DEBUG("Unhandled item %d", item);
      }
    }
    //there are some valid config items! so send the info to data
    if(data_filled)
    {
      QCRIL_LOG_INFO("Data req ptr is filled, sending request to data!");
#ifndef QMI_RIL_UTF
       std::shared_ptr<rildata::SetQualityMeasurementMessage> msg =
            std::make_shared<rildata::SetQualityMeasurementMessage>(data_set_quality_req);
       if(msg)
       {
           RadioConfigDataSetQualityMeasurementCallback cb("set-cb-token", *params_ptr, resp_data);
           msg->setCallback(&cb);
           msg->dispatch();
       }
#endif
    }
  }while(FALSE);

  if(!data_filled)
  {
    qcril_qmi_radio_config_socket_send( (RIL_Token)params_ptr->t,
          com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
          com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_SET_CONFIG,
          0, com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
          (void*)&resp_data, msg_len);
  }

  if (resp_data.settings != NULL)
  {
    qcril_free(resp_data.settings);
  }
  QCRIL_LOG_FUNC_RETURN();
}


/**
 * Analyzes and processes the get_config request data.
 */
void qcril_qmi_radio_config_dispatch_get_request
(
  const qcril_request_params_type *const params_ptr,
  qcril_request_return_type *const ret_ptr /*!< Output parameter */
)
{
  com_qualcomm_qti_radioconfiginterface_ConfigMsg resp_data;
  memset(&resp_data, 0, sizeof(com_qualcomm_qti_radioconfiginterface_ConfigMsg));
  size_t msg_len = 0;
  uint8_t send_response = TRUE;

  QCRIL_NOTUSED(params_ptr);
  QCRIL_NOTUSED(ret_ptr);
  QCRIL_LOG_FUNC_ENTRY();
  if(params_ptr == NULL || ret_ptr == NULL)
  {
    QCRIL_LOG_ERROR("Invalid arguments, cannot process further..");
    return;
  }

  do {
    com_qualcomm_qti_radioconfiginterface_ConfigMsg* config_msg_ptr =
      (com_qualcomm_qti_radioconfiginterface_ConfigMsg*)(params_ptr->data);
    if(config_msg_ptr == NULL)
    {
      //To return NULL RESPONSE?? or to handle such case in Java layer
      //could be that anotgher client(not our java service) might send it this way.
      //HOW TO HANDLE THIS?
      QCRIL_LOG_ERROR("No config items in the message!");
      break; //or send empty response with error?
    }

    QCRIL_LOG_INFO("GET_RADIO_CONFIG_REQ with %d config items.", config_msg_ptr->settings_count);

    //HARDCODED, NEEDS TO CHANGE WHILE MERGING.
    //THIS IS FOR TEST PURPOSE, BEFORE THE FINAL CHANGE IS READY.
    size_t i;
    com_qualcomm_qti_radioconfiginterface_ConfigItemMsg temp;
    qcril_qmi_radio_config_item_type item;

    //response ptr
    resp_data.settings_count = config_msg_ptr->settings_count;
    resp_data.settings = (com_qualcomm_qti_radioconfiginterface_ConfigItemMsg*)qcril_malloc(config_msg_ptr->settings_count *
        sizeof(com_qualcomm_qti_radioconfiginterface_ConfigItemMsg));
    if(resp_data.settings == NULL)
    {
      resp_data.settings_count = 0;
      QCRIL_LOG_ERROR("malloc failed");
      break;
    }
    msg_len = sizeof(resp_data);

    if (qcril_qmi_lte_direct_disc_dispatcher_req_handler(params_ptr->t, config_msg_ptr, &resp_data))
    {
      QCRIL_LOG_INFO("Handled in qcril_qmi_lte_direct_disc_dispatcher; completed!!!");
      send_response = FALSE;
      break;
    }

    for(i = 0; i < config_msg_ptr->settings_count; i++)
    {
      temp = config_msg_ptr->settings[i];
      item = qcril_qmi_radio_config_map_socket_item_to_config_item(temp.item);
      resp_data.settings[i].item = temp.item;
      resp_data.settings[i].has_errorCause = TRUE;
      resp_data.settings[i].errorCause = com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_NO_ERR;

      QCRIL_LOG_INFO("item id: %d to be set to: %d", item, temp.intValue);

      switch(item)
      {
        default:
          QCRIL_LOG_ERROR("Unknown item!!");
          resp_data.settings[i].errorCause =
                com_qualcomm_qti_radioconfiginterface_ConfigFailureCause_CONFIG_ITEM_NOT_PRESENT;
          break;
      }
    }
  } while (FALSE);

  if (send_response)
  {
    qcril_qmi_radio_config_socket_send( (RIL_Token)params_ptr->t,
        com_qualcomm_qti_radioconfiginterface_MessageType_RADIO_CONFIG_MSG_RESPONSE,
        com_qualcomm_qti_radioconfiginterface_MessageId_RADIO_CONFIG_GET_CONFIG,
        0, com_qualcomm_qti_radioconfiginterface_Error_RADIO_CONFIG_ERR_SUCCESS,
        (void*)&resp_data, msg_len);
  }

  if(resp_data.settings != NULL)
  {
    qcril_free(resp_data.settings);
  }
  QCRIL_LOG_FUNC_RETURN();
}

#endif
