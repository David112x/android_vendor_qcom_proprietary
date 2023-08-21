/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/

#define TAG "RILQ"

#include "LteDirectModule.h"
#include <framework/Log.h>
#include <framework/Module.h>
#include <framework/ModuleLooper.h>
#include <framework/QcrilInitMessage.h>

#include "qcril_memory_management.h"
#include "qcril_qmi_client.h"
#include "qcrili.h"

#include "modules/qmi/EndpointStatusIndMessage.h"
#include "modules/qmi/LteModemEndPoint.h"
#include "modules/qmi/ModemEndPointFactory.h"
#include "modules/qmi/QmiIndMessage.h"
#include "modules/qmi/QmiSetupRequestCallback.h"

#include "modules/nas/qcril_qmi_nas.h"

#include <interfaces/lte_direct/QcRilUnsolAuthorizationResultMessage.h>
#include <interfaces/lte_direct/QcRilUnsolDeviceCapabilityChangedMessage.h>
#include <interfaces/lte_direct/QcRilUnsolExpressionStatusMessage.h>
#include <interfaces/lte_direct/QcRilUnsolMatchEventMessage.h>
#include <interfaces/lte_direct/QcRilUnsolPskExpirtedMessage.h>
#include <interfaces/lte_direct/QcRilUnsolReceptionStatusMessage.h>
#include <interfaces/lte_direct/QcRilUnsolServiceStatusMessage.h>
#include <interfaces/lte_direct/QcRilUnsolTransmissionStatusMessage.h>

static qcril_qmi_lte_direct_disc_overview_type lte_direct_disc_overview;

static RIL_Errno qcril_qmi_lte_direct_disc_init(void);
static void qcril_qmi_lte_direct_disc_terminate_all_apps(void);
static qcril_qmi_lte_direct_disc_exec_overview_type *
qcril_qmi_lte_direct_disc_add_to_exec_overview(const char *os_app_id, const char *exp, uint32_t op);
static void qcril_qmi_lte_direct_disc_remove_from_exec_overview(const char *os_app_id,
                                                                const char *exp, uint32_t op);

static load_module<LteDirectModule> sLteDirectModule;

LteDirectModule *getLteDirectModule() { return &(sLteDirectModule.get_module()); }

//===========================================================================
// UTILITY FUNCTIONS
//===========================================================================

//===========================================================================
// mapQmiResultTypeToRil
//===========================================================================
static qcril::interfaces::lte_direct::Result
mapQmiResultTypeToRil(qmi_lte_disc_result_type_enum_v01 qmi_result) {
  qcril::interfaces::lte_direct::Result result = qcril::interfaces::lte_direct::Result::UNKNOWN;
  switch (qmi_result) {
  case QMI_LTE_DISC_SUCCESS_V01:
    result = qcril::interfaces::lte_direct::Result::SUCCESS;
    break;
  case QMI_LTE_DISC_IN_PROGRESS_V01:
    result = qcril::interfaces::lte_direct::Result::IN_PROGRESS;
    break;
  case QMI_LTE_DISC_ERR_INVALID_EXPRESSION_SCOPE_V01:
    result = qcril::interfaces::lte_direct::Result::INVALID_EXPRESSION_SCOPE;
    break;
  case QMI_LTE_DISC_ERR_UNKNOWN_EXPRESSION_V01:
    result = qcril::interfaces::lte_direct::Result::UNKNOWN_EXPRESSION;
    break;
  case QMI_LTE_DISC_ERR_INVALID_DISCOVERY_TYPE_V01:
    result = qcril::interfaces::lte_direct::Result::INVALID_DISCOVERY_TYPE;
    break;
  case QMI_LTE_DISC_ERR_SERVICE_UNAVAILABLE_V01:
    result = qcril::interfaces::lte_direct::Result::SERVICE_NOT_AVAILABLE;
    break;
  case QMI_LTE_DISC_ERR_APP_AUTH_FAILURE_V01:
    result = qcril::interfaces::lte_direct::Result::APP_AUTH_FAILURE;
    break;
  case QMI_LTE_DISC_ERR_FEATURE_NOT_SUPPORTED_V01:
    result = qcril::interfaces::lte_direct::Result::NOT_SUPPORTED;
    break;
  case QMI_LTE_DISC_FAILURE_V01:
  case QMI_LTE_DISC_ERR_UNKNOWN_V01:
  case QMI_LTE_DISC_ERR_NETWORK_TRANS_FAILURE_V01:
  case QMI_LTE_DISC_ERR_MALFORMED_PC3_MSG_V01:
  case QMI_LTE_DISC_ERR_INVALID_TX_ID_V01:
  case QMI_LTE_DISC_ERR_VALIDITY_TIME_EXPIRED_V01:
  default:
    result = qcril::interfaces::lte_direct::Result::GENERIC_FAILURE;
    break;
  }
  return result;
} // mapQmiResultTypeToRil

//===========================================================================
// mapQmiCategoryToRil
//===========================================================================
static qcril::interfaces::lte_direct::Category
mapQmiCategoryToRil(qmi_lte_disc_category_type_enum_v01 in) {
  qcril::interfaces::lte_direct::Category out = qcril::interfaces::lte_direct::Category::UNKNOWN;
  switch (in) {
  case QMI_LTE_DISC_CATEGORY_HIGH_V01:
    out = qcril::interfaces::lte_direct::Category::HIGH;
    break;
  case QMI_LTE_DISC_CATEGORY_MEDIUM_V01:
    out = qcril::interfaces::lte_direct::Category::MEDIUM;
    break;
  case QMI_LTE_DISC_CATEGORY_LOW_V01:
    out = qcril::interfaces::lte_direct::Category::LOW;
    break;
  case QMI_LTE_DISC_CATEGORY_VERY_LOW_V01:
    out = qcril::interfaces::lte_direct::Category::VERY_LOW;
    break;
  case QMI_LTE_DISC_CATEGORY_INVALID_V01:
    out = qcril::interfaces::lte_direct::Category::INVALID;
    break;
  default:
    out = qcril::interfaces::lte_direct::Category::UNKNOWN;
    break;
  }
  QCRIL_LOG_INFO("SUCCESS: in = %d is mapped to out %d", in, out);
  return out;
} // mapQmiCategoryToRil

//===========================================================================
// mapRilCategoryToQmi
//===========================================================================
static boolean mapRilCategoryToQmi(qcril::interfaces::lte_direct::Category in,
                                   qmi_lte_disc_category_type_enum_v01 *out) {
  boolean ret = FALSE;

  if (out) {
    ret = TRUE;
    switch (in) {
    case qcril::interfaces::lte_direct::Category::HIGH:
      *out = QMI_LTE_DISC_CATEGORY_HIGH_V01;
      break;
    case qcril::interfaces::lte_direct::Category::MEDIUM:
      *out = QMI_LTE_DISC_CATEGORY_MEDIUM_V01;
      break;
    case qcril::interfaces::lte_direct::Category::LOW:
      *out = QMI_LTE_DISC_CATEGORY_LOW_V01;
      break;
    case qcril::interfaces::lte_direct::Category::VERY_LOW:
      *out = QMI_LTE_DISC_CATEGORY_VERY_LOW_V01;
      break;
    case qcril::interfaces::lte_direct::Category::INVALID:
      *out = QMI_LTE_DISC_CATEGORY_INVALID_V01;
      break;
    default:
      ret = FALSE;
      break;
    }
  }

  QCRIL_LOG_INFO("ret = %d", ret);
  if (ret) {
    QCRIL_LOG_INFO("SUCCESS: in = %d is mapped to out %d", in, *out);
  }
  return ret;
} // mapRilCategoryToQmi

//===========================================================================
// mapRilDiscoveryTypeToQmi
//===========================================================================
static boolean mapRilDiscoveryTypeToQmi(qcril::interfaces::lte_direct::DiscoveryType discovery_type,
                                        qmi_lte_discovery_type_enum_v01 *qmi_discovery_type) {
  boolean ret = FALSE;

  if (qmi_discovery_type) {
    ret = TRUE;
    switch (discovery_type) {
    case qcril::interfaces::lte_direct::DiscoveryType::OPEN:
      *qmi_discovery_type = QMI_LTE_DISC_DISCOVERY_OPEN_V01;
      break;
    case qcril::interfaces::lte_direct::DiscoveryType::RESTRICTED:
      *qmi_discovery_type = QMI_LTE_DISC_DISCOVERY_RESTRICTED_V01;
      break;
    default:
      ret = FALSE;
      break;
    }
    QCRIL_LOG_INFO("discovery type: %d mapped to qmi discovery type: %d", discovery_type,
                   *qmi_discovery_type);
  }

  QCRIL_LOG_INFO("ret = %d", ret);
  return ret;
} // mapRilDiscoveryTypeToQmi

//===========================================================================
// mapQmiRangeToRil
//===========================================================================
static qcril::interfaces::lte_direct::Range
mapQmiRangeToRil(qmi_lte_disc_announcing_policy_range_enum_v01 in) {
  qcril::interfaces::lte_direct::Range out = qcril::interfaces::lte_direct::Range::UNKNOWN;

  switch (in) {
  case QMI_LTE_DISC_ANNOUNCING_POLICY_INVALID_V01:
    out = qcril::interfaces::lte_direct::Range::INVALID;
    break;
  case QMI_LTE_DISC_ANNOUNCING_POLICY_SHORT_V01:
    out = qcril::interfaces::lte_direct::Range::SHORT;
    break;
  case QMI_LTE_DISC_ANNOUNCING_POLICY_MEDIUM_V01:
    out = qcril::interfaces::lte_direct::Range::MEDIUM;
    break;
  case QMI_LTE_DISC_ANNOUNCING_POLICY_LONG_V01:
    out = qcril::interfaces::lte_direct::Range::LONG;
    break;
  case QMI_LTE_DISC_ANNOUNCING_POLICY_RESERVED_V01:
    out = qcril::interfaces::lte_direct::Range::RESERVED;
    break;
  default:
    out = qcril::interfaces::lte_direct::Range::UNKNOWN;
    break;
  }
  QCRIL_LOG_INFO("SUCCESS: in = %d is mapped to out %d", in, out);
  return out;
} // mapQmiRangeToRil

//===========================================================================
// mapRilRangeToQmi
//===========================================================================
static boolean mapRilRangeToQmi(qcril::interfaces::lte_direct::Range in,
                                qmi_lte_disc_announcing_policy_range_enum_v01 &out) {
  boolean ret = TRUE;
  switch (in) {
  case qcril::interfaces::lte_direct::Range::INVALID:
    out = QMI_LTE_DISC_ANNOUNCING_POLICY_INVALID_V01;
    break;
  case qcril::interfaces::lte_direct::Range::SHORT:
    out = QMI_LTE_DISC_ANNOUNCING_POLICY_SHORT_V01;
    break;
  case qcril::interfaces::lte_direct::Range::MEDIUM:
    out = QMI_LTE_DISC_ANNOUNCING_POLICY_MEDIUM_V01;
    break;
  case qcril::interfaces::lte_direct::Range::LONG:
    out = QMI_LTE_DISC_ANNOUNCING_POLICY_LONG_V01;
    break;
  case qcril::interfaces::lte_direct::Range::RESERVED:
    out = QMI_LTE_DISC_ANNOUNCING_POLICY_RESERVED_V01;
    break;
  default:
    ret = FALSE;
    break;
  }
  QCRIL_LOG_INFO("ret = %d", ret);
  if (ret) {
    QCRIL_LOG_INFO("SUCCESS: ril_range = %d is mapped to qmi_range %d", in, out);
  }
  return ret;
} // mapRilRangeToQmi

//===========================================================================
// mapRilPlmnToQmi
//
//  QMI PLMN is expected to be in the below format:
//  From lte_v01.h interface file:
//
//  A PLMN ID is defined to be the combination of a Mobile Country Code (MCC) and
//  Mobile Network Code (MNC). A PLMN ID is stored in three octets as below:
//
//
//                                        Bit
//                 |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |
//                 |===============================================|
//         octet 1 |      MCC Digit 2      |      MCC Digit 1      |
//                 |-----------------------------------------------|
//         octet 2 |      MNC Digit 3      |      MCC Digit 3      |
//                 |-----------------------------------------------|
//         octet 3 |      MNC Digit 2      |      MNC Digit 1      |
//                 |===============================================|
//
//===========================================================================
boolean mapRilPlmnToQmi(qcril::interfaces::lte_direct::Plmn &in,
                        qmi_lte_disc_plmn_id_s_type_v01 &out) {
  boolean ret = FALSE;
  qmi_lte_disc_plmn_id_s_type_v01 result_qmi_plmn;

  if (in.hasMcc() && !in.getMcc().empty() && in.hasMnc() && !in.getMnc().empty()) {
    memset(&result_qmi_plmn.plmn_id, 0xFF, sizeof(result_qmi_plmn.plmn_id));

    std::string mcc = in.getMcc();
    std::string mnc = in.getMnc();

    int mcc_len = mcc.size();
    int mnc_len = mnc.size();
    if (mcc_len == 3) {
      uint8_t d1 = mcc[0] - '0';
      uint8_t d2 = mcc[1] - '0';
      // Validate digit beteen 0 and 9
      if ((d1 <= 9) && (d2 <= 9)) {
        result_qmi_plmn.plmn_id[0] = (((0x0F & d2) << 4) | (0x0F & d1));
      }

      d1 = mcc[2] - '0';
      d2 = 0xFF;
      // MNC Digit 3
      if (mnc_len == 3) {
        d2 = mnc[2] - '0';
      }
      // Validate digit beteen 0 and 9 (d2 can be 0xFF)
      if ((d1 <= 9) && (d2 == 0xFF || d2 <= 9)) {
        result_qmi_plmn.plmn_id[1] = (((0x0F & d2) << 4) | (0x0F & d1));
      }

      if (mnc_len >= 2) {
        d1 = mnc[0] - '0';
        d2 = mnc[1] - '0';
      }
      // Validate digit beteen 0 - 9
      if ((d1 <= 9) && (d2 <= 9)) {
        result_qmi_plmn.plmn_id[2] = (((0x0F & d2) << 4) | (0x0F & d1));
      }
      ret = TRUE;
    }
  } else {
    QCRIL_LOG_ERROR("invalid input PLMN");
  }

  if (ret == TRUE) {
    memcpy(&out, &result_qmi_plmn, sizeof(result_qmi_plmn));
    QCRIL_LOG_DEBUG("ril_plmn = %s, qmi_plmn: 0x%x, 0x%x, 0x%x",
                    qcril::interfaces::lte_direct::toString(in).c_str(), out.plmn_id[0],
                    out.plmn_id[1], out.plmn_id[2]);
  }

  return ret;
} // mapRilPlmnToQmi

//===========================================================================
// mapQmiPlmnToRil
//===========================================================================
static std::shared_ptr<qcril::interfaces::lte_direct::Plmn>
mapQmiPlmnToRil(qmi_lte_disc_plmn_id_s_type_v01 &qmi_plmn) {
  char mcc[4] = {0};
  char mnc[4] = {0};

  // plmn_id[0] is |      MCC Digit 2      |      MCC Digit 1      |
  uint8_t d = (qmi_plmn.plmn_id[0] & 0x0F);
  mcc[0] = d + '0';

  d = ((qmi_plmn.plmn_id[0] >> 4) & 0x0F);
  mcc[1] = d + '0';

  // plmn_id[1] is |      MNC Digit 3      |      MCC Digit 3      |
  d = (qmi_plmn.plmn_id[1] & 0x0F);
  mcc[2] = d + '0';

  d = ((qmi_plmn.plmn_id[1] >> 4) & 0x0F);
  if (d != 0xF) {
    mnc[2] = d + '0';
  }

  // plmn_id[2] |      MNC Digit 2      |      MNC Digit 1      |
  d = (qmi_plmn.plmn_id[2] & 0x0F);
  mnc[0] = d + '0';

  d = ((qmi_plmn.plmn_id[2] >> 4) & 0x0F);
  mnc[1] = d + '0';

  auto ril_plmn = std::make_shared<qcril::interfaces::lte_direct::Plmn>();
  if (ril_plmn) {
    ril_plmn->setMcc(mcc);
    ril_plmn->setMnc(mnc);

    QCRIL_LOG_DEBUG("qmi_plmn: 0x%x, 0x%x, 0x%x, ril_plmn = %s", qmi_plmn.plmn_id[0],
                    qmi_plmn.plmn_id[1], qmi_plmn.plmn_id[2],
                    qcril::interfaces::lte_direct::toString(*ril_plmn).c_str());
  }

  return ril_plmn;
} // mapQmiPlmnToRil

//===========================================================================
// qmi_client_lte_send_async
//===========================================================================
static qmi_client_error_type qmi_client_lte_send_async(unsigned long msg_id, void *req_ptr,
                                                       int req_struct_len, int resp_struct_len,
                                                       qmiAsyncCbType resp_cb, void *resp_cb_data) {
  qmi_client_error_type rc = QMI_INTERNAL_ERR;

  QCRIL_LOG_FUNC_ENTRY();

  rc = ModemEndPointFactory<LteModemEndPoint>::getInstance().buildEndPoint()->sendRawAsync(
      msg_id, req_ptr, req_struct_len, resp_struct_len, resp_cb, resp_cb_data,
      getLteDirectModule());

  QCRIL_LOG_FUNC_RETURN_WITH_RET(rc);
  return rc;
}

//=========================================================================
// sendUnsolExpressionStatus
//=========================================================================
static void sendUnsolExpressionStatus(const char *os_app_id,
                                      qmi_lte_disc_prose_app_id_v01 *expression,
                                      qmi_lte_disc_result_type_enum_v01 expression_result) {
  auto msg = std::make_shared<QcRilUnsolExpressionStatusMessage>();
  boolean sendUnsol = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  do {
    if (msg == nullptr) {
      QCRIL_LOG_ERROR("msg is null");
      break;
    }
    if (os_app_id != NULL) {
      msg->setOsAppId(os_app_id);
    } else {
      QCRIL_LOG_ERROR("Empty OS App ID; os_app_id");
      break;
    }
    if (expression != NULL) {
      msg->setExpression(expression->prose_app_id_name);
    } else {
      QCRIL_LOG_ERROR("Empty Expression;");
      break;
    }
    msg->setResult(mapQmiResultTypeToRil(expression_result));
    // Good to go; send UNSOL_RESPONSE_EXPRESSION_STATUS
    sendUnsol = TRUE;
  } while (FALSE);

  if (sendUnsol && msg) {
    Dispatcher::getInstance().dispatchSync(msg);
  }
  QCRIL_LOG_FUNC_RETURN();
} // sendUnsolExpressionStatus

//=========================================================================
// sendUnsolAuthorizationResult
//===========================================================================
static void sendUnsolAuthorizationResult(const char *os_app_id,
                                         qmi_lte_disc_result_type_enum_v01 qmi_auth_result) {
  auto msg = std::make_shared<QcRilUnsolAuthorizationResultMessage>();
  boolean sendUnsol = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  do {
    if (msg == nullptr) {
      QCRIL_LOG_ERROR("msg is null");
      break;
    }
    if (os_app_id != NULL) {
      msg->setOsAppId(os_app_id);
    } else {
      QCRIL_LOG_ERROR("Empty OS App ID; os_app_id");
      break;
    }
    msg->setResult(mapQmiResultTypeToRil(qmi_auth_result));
    // Good to go; send UNSOL_RESPONSE_AUTHORIZATION_RESULT
    sendUnsol = TRUE;
  } while (FALSE);

  if (sendUnsol && msg) {
    Dispatcher::getInstance().dispatchSync(msg);
  }

  QCRIL_LOG_FUNC_RETURN();
} /* sendUnsolAuthorizationResult */

//=========================================================================
// sendUnsolTransmissionStatus
//===========================================================================
static void sendUnsolTransmissionStatus(const char *os_app_id,
                                        qmi_lte_disc_prose_app_id_v01 *expression,
                                        uint8_t publishStatusPerExpr) {
  auto msg = std::make_shared<QcRilUnsolTransmissionStatusMessage>();
  boolean sendUnsol = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  do {
    if (msg == nullptr) {
      QCRIL_LOG_ERROR("msg is null");
      break;
    }
    if (os_app_id != NULL) {
      msg->setOsAppId(os_app_id);
    } else {
      QCRIL_LOG_ERROR("Empty OS App ID; os_app_id");
      break;
    }
    if (expression != NULL) {
      msg->setExpression(expression->prose_app_id_name);
    } else {
      QCRIL_LOG_ERROR("Empty Expression;");
      break;
    }
    if (publishStatusPerExpr == 0) {
      msg->setStatus(0); // Transmission is completed
    } else {
      msg->setStatus(1); // Pending Transmission
    }
    // Good to go; send UNSOL_RESPONSE_TRANSMISSION_STATUS
    sendUnsol = TRUE;
  } while (FALSE);

  if (sendUnsol && msg) {
    Dispatcher::getInstance().dispatchSync(msg);
  }
  QCRIL_LOG_FUNC_RETURN();
} /* sendUnsolTransmissionStatus */

//=========================================================================
// sendUnsolReceptionStatus
//===========================================================================
static void sendUnsolReceptionStatus(const char *os_app_id,
                                     qmi_lte_disc_prose_app_id_v01 *expression,
                                     uint8_t subscribeStatusPerExpr) {
  auto msg = std::make_shared<QcRilUnsolReceptionStatusMessage>();
  boolean sendUnsol = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  do {
    if (msg == nullptr) {
      QCRIL_LOG_ERROR("msg is null");
      break;
    }
    if (os_app_id != NULL) {
      msg->setOsAppId(os_app_id);
    } else {
      QCRIL_LOG_ERROR("Empty OS App ID; os_app_id");
      break;
    }
    if (expression != NULL) {
      msg->setExpression(expression->prose_app_id_name);
    } else {
      QCRIL_LOG_ERROR("Empty Expression;");
      break;
    }
    if (subscribeStatusPerExpr == 0) {
      msg->setStatus(0); // Transmission is completed
    } else {
      msg->setStatus(1); // Pending Transmission
    }
    // Good to go; send UNSOL_RESPONSE_RECEPTION_STATUS
    sendUnsol = TRUE;
  } while (FALSE);

  if (sendUnsol && msg) {
    Dispatcher::getInstance().dispatchSync(msg);
  }

  QCRIL_LOG_FUNC_RETURN();
} // sendUnsolReceptionStatus

//=========================================================================
// sendUnsolServiceStatus
//===========================================================================
static void sendUnsolServiceStatus(uint8_t PublishAllowed_valid, uint8_t PublishAllowed,
                                   uint8_t SubscribeAllowed_valid, uint8_t SubscribeAllowed) {
  auto msg = std::make_shared<QcRilUnsolServiceStatusMessage>();
  boolean sendUnsol = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  do {
    if (msg == nullptr) {
      QCRIL_LOG_ERROR("msg is null");
      break;
    }
    if (PublishAllowed_valid) {
      msg->setPublishAllowed(PublishAllowed);
    }
    if (SubscribeAllowed_valid) {
      msg->setSubscribeAllowed(SubscribeAllowed);
    }
    // Good to go; send UNSOL_RESPONSE_SERVICE_STATUS
    sendUnsol = TRUE;
  } while (FALSE);

  if (sendUnsol && msg) {
    Dispatcher::getInstance().dispatchSync(msg);
  }

  QCRIL_LOG_FUNC_RETURN();
} // sendUnsolServiceStatus

//=========================================================================
// qcril_qmi_lte_direct_disc_on_radio_state_change
//===========================================================================
/*!
  @brief
  Notify the service status as not Allowed in case if the device
  is not in online mode.
 */
/*=========================================================================*/
RIL_Errno qcril_qmi_lte_direct_disc_on_radio_state_change(void) {
  RIL_Errno res = RIL_E_SUCCESS;
  boolean is_in_online_mode = qcril_qmi_nas_dms_is_in_online_mode();
  QCRIL_LOG_INFO("is_in_online_mode = %d", is_in_online_mode);
  if (!is_in_online_mode) {
    sendUnsolServiceStatus(TRUE, FALSE, TRUE, FALSE);
  }
  return res;
}

//===========================================================================
//===========================================================================

//===========================================================================
// LteDirectModule Constructor
//===========================================================================
LteDirectModule::LteDirectModule() : AddPendingMessageList("LteDirectModule") {
  mName = "LteDirectModule";
  mLooper = nullptr;

  using std::placeholders::_1;
  mMessageHandler = {
      HANDLER(QcrilInitMessage, LteDirectModule::handleQcrilInitMessage),

      {REG_MSG("LTE_QMI_IND"), std::bind(&LteDirectModule::handleQmiIndMessage, this, _1)},
      // QMI Async response
      HANDLER(QmiAsyncResponseMessage, LteDirectModule::handleQmiAsyncRespMessage),
      // End Point Status Indication
      {REG_MSG("LTE_ENDPOINT_STATUS_IND"),
       std::bind(&LteDirectModule::handleEndpointStatusIndMessage, this, _1)},

      HANDLER(QcRilRequestGetLtedConfigMessage,
              LteDirectModule::handleQcRilRequestGetLtedConfigMessage),
      HANDLER(QcRilRequestGetLtedCategoryMessage,
              LteDirectModule::handleQcRilRequestGetLtedCategoryMessage),
      HANDLER(QcRilRequestSetLtedCategoryMessage,
              LteDirectModule::handleQcRilRequestSetLtedCategoryMessage),
      HANDLER(QcRilRequestSetLtedConfigMessage,
              LteDirectModule::handleQcRilRequestSetLtedConfigMessage),

      HANDLER(QcRilRequestLteDirectInitializeMessage,
              LteDirectModule::handleQcRilRequestLteDirectInitializeMessage),
      HANDLER(QcRilRequestGetServiceStatusMessage,
              LteDirectModule::handleQcRilRequestGetServiceStatusMessage),
      HANDLER(QcRilRequestGetDeviceCapabilityMessage,
              LteDirectModule::handleQcRilRequestGetDeviceCapabilityMessage),
      HANDLER(QcRilRequestPublishMessage, LteDirectModule::handleQcRilRequestPublishMessage),
      HANDLER(QcRilRequestCancelPublishMessage,
              LteDirectModule::handleQcRilRequestCancelPublishMessage),
      HANDLER(QcRilRequestSubscribeMessage, LteDirectModule::handleQcRilRequestSubscribeMessage),
      HANDLER(QcRilRequestCancelSubscribeMessage,
              LteDirectModule::handleQcRilRequestCancelSubscribeMessage),
      HANDLER(QcRilRequestTerminateMessage, LteDirectModule::handleQcRilRequestTerminateMessage),
  };
  ModemEndPointFactory<LteModemEndPoint>::getInstance().buildEndPoint();
}

//===========================================================================
// LteDirectModule Destructor
//===========================================================================
LteDirectModule::~LteDirectModule() { mReady = false; }

void LteDirectModule::init() {
  Module::init();
  // qcril_qmi_lte_direct_init(); // pre_init
  // Initializations complete.
  mReady = true;
}

//===========================================================================
// isReady
//===========================================================================
bool LteDirectModule::isReady() { return mReady; }

//===========================================================================
// handleQcrilInitMessage
//===========================================================================
void LteDirectModule::handleQcrilInitMessage(std::shared_ptr<QcrilInitMessage> msg) {
  Log::getInstance().d("[" + mName + "]: Handling msg = " + msg->dump());
  Log::getInstance().d("[" + mName + "]: initialize LTE Modem Endpoint module");
  /* Init QMI LTE services.*/
  QmiSetupRequestCallback qmiLteSetupCallback("LteDirectModule-Token");
  ModemEndPointFactory<LteModemEndPoint>::getInstance().buildEndPoint()->requestSetup(
      "LteDirectModule-Token", &qmiLteSetupCallback);
}

//===========================================================================
// handleEndpointStatusIndMessage
//===========================================================================
void LteDirectModule::handleEndpointStatusIndMessage(std::shared_ptr<Message> msg) {
  auto shared_indMsg(std::static_pointer_cast<EndpointStatusIndMessage>(msg));

  Log::getInstance().d("[" + mName + "]: Handling msg = " + msg->dump());

  if (shared_indMsg->getState() == ModemEndPoint::State::OPERATIONAL) {
    QCRIL_LOG_INFO("Initing  LteDirectDiscovery client...");
    qcril_qmi_lte_direct_disc_init();
    mReady = true;
  } else {
    mReady = false;
    // clean up
    // qcril_qmi_lte_cleanup();
  }
}

//===========================================================================
// handleQmiIndMessage
//===========================================================================
void LteDirectModule::handleQmiIndMessage(std::shared_ptr<Message> msg) {
  Log::getInstance().d("[" + mName + "]: Handling msg = " + msg->dump());
  auto shared_indMsg(std::static_pointer_cast<QmiIndMessage>(msg));
  QmiIndMsgDataStruct *indData = shared_indMsg->getData();
  if (indData != nullptr) {
    switch (indData->msgId) {
    case QMI_LTE_DISC_NOTIFICATION_IND_V01:
      handleLteDiscNotificationInd((qmi_lte_disc_notification_ind_msg_v01 *)(indData->indData));
      break;
    case QMI_LTE_DISC_BROADCAST_NOTIFICATION_IND_V01:
      handleLteDiscBroadcastNotificationInd(
          (qmi_lte_disc_broadcast_notification_ind_msg_v01 *)(indData->indData));
      break;
    case QMI_LTE_DISC_MATCH_NOTIFICATION_IND_V01:
      handleLteDiscMatchNotificationInd(
          (qmi_lte_disc_match_notification_ind_msg_v01 *)(indData->indData));
      break;
    case QMI_LTE_SUBSCRIPTION_INFO_IND_V01:
      handleLteDiscSubscriptionInfoInd((qmi_lte_subscription_info_ind_msg_v01 *)(indData->indData));
      break;
    case QMI_LTE_DISC_PSK_EXPIRED_IND_V01:
      handleLteDiscPskExpirtedInd((qmi_lte_disc_psk_expired_ind_msg_v01 *)(indData->indData));
      break;
    default:
      QCRIL_LOG_INFO("Unknown QMI LTE indication %d", indData->msgId);
      break;
    }
  } else {
    Log::getInstance().d("Unexpected, null data from message");
  }
}

//===========================================================================
// handleLteDiscNotificationInd
//===========================================================================
void LteDirectModule::handleLteDiscNotificationInd(
    qmi_lte_disc_notification_ind_msg_v01 *notif_ind_msg) {
  uint32_t op_mask = QCRIL_QMI_LTE_DIRECT_DISC_OP_NONE;
  boolean reset_exec_overview = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  if (notif_ind_msg != NULL) {
    QCRIL_LOG_INFO("OsAppId_valid = %d, OsAppId = %s", notif_ind_msg->OsAppId_valid,
                   notif_ind_msg->OsAppId_valid ? notif_ind_msg->OsAppId : "<not valid>");

    QCRIL_LOG_INFO("Expression_valid = %d, Expression = %s", notif_ind_msg->Expression_valid,
                   notif_ind_msg->Expression_valid ? notif_ind_msg->Expression.prose_app_id_name
                                                   : "<not valid>");

    QCRIL_LOG_INFO("ExpressionResult_valid = %d, ExpressionResult = %d",
                   notif_ind_msg->ExpressionResult_valid,
                   notif_ind_msg->ExpressionResult_valid ? notif_ind_msg->ExpressionResult : -1);

    if (notif_ind_msg->OsAppId_valid && notif_ind_msg->Expression_valid &&
        notif_ind_msg->ExpressionResult_valid) {
      sendUnsolExpressionStatus(notif_ind_msg->OsAppId, &(notif_ind_msg->Expression),
                                notif_ind_msg->ExpressionResult);

      if (notif_ind_msg->ExpressionResult != QMI_LTE_DISC_SUCCESS_V01 ||
          notif_ind_msg->ExpressionResult != QMI_LTE_DISC_IN_PROGRESS_V01) {
        reset_exec_overview = TRUE;
        op_mask = QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH | QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE;
      }
    }

    QCRIL_LOG_INFO("AuthorizationResult_valid = %d, AuthorizationResult = %d",
                   notif_ind_msg->AuthorizationResult_valid,
                   notif_ind_msg->AuthorizationResult_valid ? notif_ind_msg->AuthorizationResult
                                                            : -1);

    if (notif_ind_msg->OsAppId_valid && notif_ind_msg->AuthorizationResult_valid) {
      sendUnsolAuthorizationResult(notif_ind_msg->OsAppId, notif_ind_msg->AuthorizationResult);

      if (notif_ind_msg->AuthorizationResult != QMI_LTE_DISC_SUCCESS_V01 ||
          notif_ind_msg->AuthorizationResult != QMI_LTE_DISC_IN_PROGRESS_V01) {
        reset_exec_overview = TRUE;
        op_mask = QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH | QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE;
      }
    }

    QCRIL_LOG_INFO("PublishStatusPerExpr_valid = %d, PublishStatusPerExpr = %d",
                   notif_ind_msg->PublishStatusPerExpr_valid,
                   notif_ind_msg->PublishStatusPerExpr_valid ? notif_ind_msg->PublishStatusPerExpr
                                                             : -1);

    if (notif_ind_msg->OsAppId_valid && notif_ind_msg->Expression_valid &&
        notif_ind_msg->PublishStatusPerExpr_valid) {
      sendUnsolTransmissionStatus(notif_ind_msg->OsAppId, &(notif_ind_msg->Expression),
                                  notif_ind_msg->PublishStatusPerExpr);

      if (notif_ind_msg->PublishStatusPerExpr == 0x00) {
        reset_exec_overview = TRUE;
        op_mask |= QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH;
      }
    }

    QCRIL_LOG_INFO(
        "SubscribeStatusPerExpr_valid = %d, SubscribeStatusPerExpr = %d",
        notif_ind_msg->SubscribeStatusPerExpr_valid,
        notif_ind_msg->SubscribeStatusPerExpr_valid ? notif_ind_msg->SubscribeStatusPerExpr : -1);

    if (notif_ind_msg->OsAppId_valid && notif_ind_msg->Expression_valid &&
        notif_ind_msg->SubscribeStatusPerExpr_valid) {
      sendUnsolReceptionStatus(notif_ind_msg->OsAppId, &(notif_ind_msg->Expression),
                               notif_ind_msg->SubscribeStatusPerExpr);

      if (notif_ind_msg->SubscribeStatusPerExpr == 0x00) {
        reset_exec_overview = TRUE;
        op_mask |= QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE;
      }
    }

    if (reset_exec_overview) {
      qcril_qmi_lte_direct_disc_remove_from_exec_overview(
          notif_ind_msg->OsAppId, notif_ind_msg->Expression.prose_app_id_name, op_mask);
    }
  } else {
    QCRIL_LOG_ERROR("ind_data_ptr is NULL");
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleLteDiscBroadcastNotificationInd
//===========================================================================
void LteDirectModule::handleLteDiscBroadcastNotificationInd(
    qmi_lte_disc_broadcast_notification_ind_msg_v01 *notif_ind_msg) {
  QCRIL_LOG_FUNC_ENTRY();

  if (notif_ind_msg != NULL) {
    QCRIL_LOG_INFO("PublishAllowed_valid = %d, SubscribeAllowed_valid = %d",
                   notif_ind_msg->PublishAllowed_valid, notif_ind_msg->SubscribeAllowed_valid);
    if (notif_ind_msg->PublishAllowed_valid || notif_ind_msg->SubscribeAllowed_valid) {
      sendUnsolServiceStatus(notif_ind_msg->PublishAllowed_valid, notif_ind_msg->PublishAllowed,
                             notif_ind_msg->SubscribeAllowed_valid,
                             notif_ind_msg->SubscribeAllowed);
    }
  } else {
    QCRIL_LOG_ERROR("ind_data_ptr is NULL");
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleLteDiscMatchNotificationInd
//===========================================================================
void LteDirectModule::handleLteDiscMatchNotificationInd(
    qmi_lte_disc_match_notification_ind_msg_v01 *notif_ind_msg) {
  auto msg = std::make_shared<QcRilUnsolMatchEventMessage>();
  boolean sendUnsol = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  if (notif_ind_msg != NULL) {
    do {
      if (msg == nullptr) {
        QCRIL_LOG_ERROR("msg is null");
        break;
      }
      QCRIL_LOG_INFO("OsAppId_valid = %d, OsAppId = %s", notif_ind_msg->OsAppId_valid,
                     notif_ind_msg->OsAppId_valid ? notif_ind_msg->OsAppId : "<not valid>");

      if (notif_ind_msg->OsAppId_valid) {
        msg->setOsAppId(notif_ind_msg->OsAppId);
      } else {
        QCRIL_LOG_ERROR("Empty OS App ID;");
        break;
      }

      QCRIL_LOG_INFO("Expression_valid = %d, Expression = %s", notif_ind_msg->Expression_valid,
                     notif_ind_msg->Expression_valid ? notif_ind_msg->Expression.prose_app_id_name
                                                     : "<not valid>");

      if (notif_ind_msg->Expression_valid) {
        msg->setExpression(notif_ind_msg->Expression.prose_app_id_name);
      } else {
        QCRIL_LOG_ERROR("Empty Expression;");
        break;
      }

      QCRIL_LOG_INFO("MatchedExpression_valid = %d, MatchedExpression = %s",
                     notif_ind_msg->MatchedExpression_valid,
                     notif_ind_msg->MatchedExpression_valid
                         ? notif_ind_msg->MatchedExpression.prose_app_id_name
                         : "<not valid>");

      if (notif_ind_msg->MatchedExpression_valid) {
        msg->setMatchedExpression(notif_ind_msg->MatchedExpression.prose_app_id_name);
      } else {
        QCRIL_LOG_ERROR("Empty MatchedExpression;");
        break;
      }

      QCRIL_LOG_INFO(
          "MatchEventState_valid = %d, MatchEventState = %d", notif_ind_msg->MatchEventState_valid,
          notif_ind_msg->MatchEventState_valid ? notif_ind_msg->MatchEventState_valid : -1);
      if (notif_ind_msg->MatchEventState_valid) {
        if (notif_ind_msg->MatchEventState == 0x00) {
          msg->setState(0); // Match start
        } else {
          msg->setState(1); // Match end
        }
      }

      QCRIL_LOG_INFO("MetaDataIndex_valid = %d, MetaDataIndex = %d",
                     notif_ind_msg->MetaDataIndex_valid,
                     notif_ind_msg->MetaDataIndex_valid ? notif_ind_msg->MetaDataIndex : -1);

      if (notif_ind_msg->MetaDataIndex_valid) {
        msg->setMetaDataIndex(notif_ind_msg->MetaDataIndex);
      }

      if (notif_ind_msg->MetaData_valid) {
        msg->setMetaData(notif_ind_msg->MetaData);
      } else {
        QCRIL_LOG_ERROR("Empty MetaData;");
      }

      // Good to go; send UNSOL_RESPONSE_MATCH_EVENT
      sendUnsol = TRUE;
    } while (FALSE);

    if (sendUnsol && msg) {
      Dispatcher::getInstance().dispatchSync(msg);
    }
  } else {
    QCRIL_LOG_ERROR("ind_data_ptr is NULL");
  }

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleLteDiscSubscriptionInfoInd
//===========================================================================
void LteDirectModule::handleLteDiscSubscriptionInfoInd(
    qmi_lte_subscription_info_ind_msg_v01 *notif_ind_msg) {
  auto msg = std::make_shared<QcRilUnsolDeviceCapabilityChangedMessage>();
  boolean sendUnsol = FALSE;

  QCRIL_LOG_FUNC_ENTRY();

  if (notif_ind_msg != NULL) {
    do {
      if (msg == nullptr) {
        QCRIL_LOG_ERROR("msg is null");
        break;
      }
      QCRIL_LOG_INFO("LteDiscCapability_valid = %d, LteDiscCapability = %d",
                     notif_ind_msg->LteDiscCapability_valid, notif_ind_msg->LteDiscCapability);

      if (notif_ind_msg->LteDiscCapability_valid) {
        msg->setCapability(notif_ind_msg->LteDiscCapability);
      } else {
        QCRIL_LOG_ERROR("LteDiscCapability not valid");
        break;
      }
      // Good to go; send UNSOL_RESPONSE_DEVICE_CAPABILITY_CHANGED
      sendUnsol = TRUE;
    } while (FALSE);

    if (sendUnsol && msg) {
      Dispatcher::getInstance().dispatchSync(msg);
    }
  } else {
    QCRIL_LOG_ERROR("ind_data_ptr is NULL");
  }

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleLteDiscPskExpirtedInd
//===========================================================================
void LteDirectModule::handleLteDiscPskExpirtedInd(
    qmi_lte_disc_psk_expired_ind_msg_v01 * /*notif_ind_msg*/) {
  auto msg = std::make_shared<QcRilUnsolPskExpirtedMessage>();
  QCRIL_LOG_FUNC_ENTRY();

  if (msg) {
    Dispatcher::getInstance().dispatchSync(msg);
  }

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQmiAsyncRespMessage
//===========================================================================
void LteDirectModule::handleQmiAsyncRespMessage(std::shared_ptr<QmiAsyncResponseMessage> msg) {
  Log::getInstance().d("[" + mName + "]: Handling msg = " + msg->dump());
  QmiAsyncRespData *asyncResp = msg->getData();
  if (asyncResp != nullptr && asyncResp->cb != nullptr) {
    asyncResp->cb(asyncResp->msgId, asyncResp->respData, asyncResp->respDataSize, asyncResp->cbData,
                  asyncResp->traspErr);
  } else {
    Log::getInstance().d("Unexpected, null data from message");
  }
}

//===========================================================================
// handleQcRilRequestLteDirectInitializeMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestLteDirectInitializeMessage(
    std::shared_ptr<QcRilRequestLteDirectInitializeMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));

  qcril_qmi_lte_direct_disc_terminate_all_apps();

  // Send success response
  auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(RIL_E_SUCCESS, nullptr);
  msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
}

//===========================================================================
// handleQcRilRequestGetLtedConfigMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestGetLtedConfigMessage(
    std::shared_ptr<QcRilRequestGetLtedConfigMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_GET_LTED_CONFIG_REQ_V01, NULL, 0,
        sizeof(qmi_lte_disc_get_lted_config_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          std::shared_ptr<qcril::interfaces::lte_direct::GetLtedConfigResp> getConfResp = nullptr;
          qmi_lte_disc_get_lted_config_resp_msg_v01 *response =
              (qmi_lte_disc_get_lted_config_resp_msg_v01 *)(resp_c_struct.get());

          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          if (rilReqRes == RIL_E_SUCCESS) {
            getConfResp = std::make_shared<qcril::interfaces::lte_direct::GetLtedConfigResp>();
            if (getConfResp) {
              if (response->DedicatedApnName_valid) {
                QCRIL_LOG_INFO("DedicatedApnName = %s", response->DedicatedApnName);
                getConfResp->setApn(response->DedicatedApnName);
              } else {
                QCRIL_LOG_INFO("DedicatedApnName not valid");
              }

              QCRIL_LOG_INFO("AnnouncingPolicy_valid = %d, AnnouncingPolicy_len = %d",
                             response->AnnouncingPolicy_valid, response->AnnouncingPolicy_len);
              if (response->AnnouncingPolicy_valid) {
                for (uint32_t i = 0; i < response->AnnouncingPolicy_len; i++) {
                  qcril::interfaces::lte_direct::AnnouncingPolicy annPolicy = {};
                  qmi_lte_disc_announcing_policy_info_type_v01 qmi_ann_policy =
                      response->AnnouncingPolicy[i];

                  // PLMN
                  annPolicy.setPlmn(mapQmiPlmnToRil(qmi_ann_policy.PlmnId));

                  QCRIL_LOG_INFO("AnnouncingPolicy[%d]: T4005_ValidityTime = %d", i,
                                 qmi_ann_policy.T4005_ValidityTime);
                  annPolicy.setValidityTime(qmi_ann_policy.T4005_ValidityTime);

                  QCRIL_LOG_INFO("AnnouncingPolicy[%d]: Range = %d", i, qmi_ann_policy.Range);
                  qcril::interfaces::lte_direct::Range ril_range =
                      mapQmiRangeToRil(qmi_ann_policy.Range);
                  if (ril_range != qcril::interfaces::lte_direct::Range::UNKNOWN) {
                    annPolicy.setRange(ril_range);
                  }
                  getConfResp->addToAnnouncingPolicy(annPolicy);
                }
              }

              QCRIL_LOG_INFO("MonitoringPolicy_valid = %d, MonitoringPolicy_len = %d",
                             response->MonitoringPolicy_valid, response->MonitoringPolicy_len);
              if (response->MonitoringPolicy_valid) {
                for (uint32_t i = 0; i < response->MonitoringPolicy_len; i++) {
                  qcril::interfaces::lte_direct::MonitoringPolicy monPolicy = {};
                  qmi_lte_disc_monitoring_policy_info_type_v01 qmi_mon_policy =
                      response->MonitoringPolicy[i];

                  // PLMN
                  monPolicy.setPlmn(mapQmiPlmnToRil(qmi_mon_policy.plmn_id));

                  // Validity time
                  QCRIL_LOG_INFO("MonitoringPolicy[%d]: T4005_ValidityTime = %d", i,
                                 qmi_mon_policy.T4005_ValidityTime);
                  monPolicy.setValidityTime(qmi_mon_policy.T4005_ValidityTime);

                  // Remaining time
                  QCRIL_LOG_INFO("MonitoringPolicy[%d]: RemainingTime = %d", i,
                                 qmi_mon_policy.RemainingTime);
                  monPolicy.setRemainingTime(qmi_mon_policy.RemainingTime);
                  getConfResp->addToMonitoringPolicy(monPolicy);
                }
              }
            }
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestGetDeviceCapabilityMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, getConfResp);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestSetLtedConfigMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestSetLtedConfigMessage(
    std::shared_ptr<QcRilRequestSetLtedConfigMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  qmi_lte_disc_set_lted_config_req_msg_v01 req = {};
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (!msg->hasOsId()) {
      QCRIL_LOG_ERROR("Invalid OS ID");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }

    req.OsId.value_1_64 = msg->getOsId().getLsb();
    req.OsId.value_65_128 = msg->getOsId().getMsb();

    if (msg->hasApn() && !msg->getApn().empty()) {
      if (msg->getApn().size() > QMI_LTE_DISC_DEDICATED_APN_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid APN length: %d", msg->getApn().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      req.DedicatedApnName_valid = TRUE;
      memcpy(&req.DedicatedApnName, msg->getApn().c_str(), msg->getApn().size());
    }

    if (msg->hasAnnouncingPolicy() && !msg->getAnnouncingPolicy().empty()) {
      QCRIL_LOG_INFO("announcingPolicy_count = %d", msg->getAnnouncingPolicy().size());

      req.AnnouncingPolicyList_valid = TRUE;
      req.AnnouncingPolicyList_len = msg->getAnnouncingPolicy().size();

      for (uint32_t i = 0; i < msg->getAnnouncingPolicy().size(); i++) {
        const qcril::interfaces::lte_direct::AnnouncingPolicy &ann_policy = msg->getAnnouncingPolicy()[i];

        // PLMN
        if (ann_policy.hasPlmn()) {
          mapRilPlmnToQmi(*(ann_policy.getPlmn()), req.AnnouncingPolicyList[i].PlmnId);
        }

        if (ann_policy.hasValidityTime()) {
          // T4005 ValidityTime
          req.AnnouncingPolicyList[i].T4005_ValidityTime = ann_policy.getValidityTime();
          // QCRIL_LOG_INFO("announcingPolicy[%d]: validityTime = %d", i,
          // ann_policy.getValidityTime());
        }

        if (ann_policy.hasRange()) {
          // Range
          // QCRIL_LOG_INFO("announcingPolicy[%d]: range = %d", i, ann_policy.range);
          mapRilRangeToQmi(ann_policy.getRange(), req.AnnouncingPolicyList[i].Range);
        }
      }
    } else {
      QCRIL_LOG_INFO("empty announcingPolicy");
    }

    if (msg->hasMonitoringPolicy() && !msg->getMonitoringPolicy().empty()) {
      QCRIL_LOG_INFO("monitoringPolicy_count = %d", msg->getMonitoringPolicy().size());

      req.MonitoringPolicyList_valid = TRUE;
      req.MonitoringPolicyList_len = msg->getMonitoringPolicy().size();

      for (uint32_t i = 0; i < msg->getMonitoringPolicy().size(); i++) {
        const qcril::interfaces::lte_direct::MonitoringPolicy &mon_policy = msg->getMonitoringPolicy()[i];

        // PLMN
        if (mon_policy.hasPlmn()) {
          mapRilPlmnToQmi(*(mon_policy.getPlmn()), req.MonitoringPolicyList[i].plmn_id);
        }

        if (mon_policy.hasValidityTime()) {
          // T4005 ValidityTime
          req.MonitoringPolicyList[i].T4005_ValidityTime = mon_policy.getValidityTime();
        }

        if (mon_policy.hasRemainingTime()) {
          // RemainingTime
          req.MonitoringPolicyList[i].RemainingTime = mon_policy.getRemainingTime();
        }
      }
    }
    if (msg->hasBakPassword() && !msg->getBakPassword().empty()) {
      if (msg->getBakPassword().size() < QMI_LTE_DISC_PSK_MAX_LEN_V01) {
        req.BAKPassword_valid = TRUE;
        req.BAKPassword_len = msg->getBakPassword().size();
        for (uint32_t i = 0; i < msg->getBakPassword().size(); i++) {
          req.BAKPassword[i] = msg->getBakPassword()[i];
        }
      }
    }

    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_SET_LTED_CONFIG_REQ_V01, &req, sizeof(req),
        sizeof(qmi_lte_disc_set_lted_config_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          qmi_lte_disc_set_lted_config_resp_msg_v01 *response =
              (qmi_lte_disc_set_lted_config_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestPublishMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, nullptr);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestGetLtedCategoryMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestGetLtedCategoryMessage(
    std::shared_ptr<QcRilRequestGetLtedCategoryMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  RIL_Errno rilErr = RIL_E_SUCCESS;
  qmi_lte_disc_get_lted_category_req_msg_v01 req = {};
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (msg->hasOsAppId() && !msg->getOsAppId().empty()) {
      if (msg->getOsAppId().size() > QMI_LTE_DISC_OS_APP_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid OS App ID length: %d", msg->getOsAppId().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      memcpy(&req.OsAppId, msg->getOsAppId().c_str(), msg->getOsAppId().size());
    }
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_GET_CATEGORY_REQ_V01, &req, sizeof(qmi_lte_disc_get_lted_category_req_msg_v01),
        sizeof(qmi_lte_disc_get_lted_category_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          std::shared_ptr<qcril::interfaces::lte_direct::GetLtedCategoryResp> getCatResp = nullptr;
          qmi_lte_disc_get_lted_category_resp_msg_v01 *response =
              (qmi_lte_disc_get_lted_category_resp_msg_v01 *)(resp_c_struct.get());

          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
            QCRIL_LOG_INFO("lted_category_valid = %d, lted_category = %d",
                           response->lted_category_valid, response->lted_category);
            if (response->lted_category_valid) {
              qcril::interfaces::lte_direct::Category cat =
                  mapQmiCategoryToRil(response->lted_category);
              if (cat != qcril::interfaces::lte_direct::Category::UNKNOWN) {
                getCatResp = std::make_shared<qcril::interfaces::lte_direct::GetLtedCategoryResp>();
                if (getCatResp) {
                  getCatResp->setCategory(cat);
                }
              }
            }
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestGetDeviceCapabilityMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, getCatResp);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestSetLtedCategoryMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestSetLtedCategoryMessage(
    std::shared_ptr<QcRilRequestSetLtedCategoryMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  qmi_lte_disc_set_lted_category_req_msg_v01 req = {};
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (msg->hasOsAppId() && !msg->getOsAppId().empty()) {
      if (msg->getOsAppId().size() > QMI_LTE_DISC_OS_APP_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid OS App ID length: %d", msg->getOsAppId().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      memcpy(&req.OsAppId, msg->getOsAppId().c_str(), msg->getOsAppId().size());
    }
    if (msg->hasCategory()) {
      req.lted_category_valid = mapRilCategoryToQmi(msg->getCategory(), &(req.lted_category));
    }

    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_SET_LTED_CATEGORY_REQ_V01, &req, sizeof(req),
        sizeof(qmi_lte_disc_set_lted_category_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          qmi_lte_disc_set_lted_category_resp_msg_v01 *response =
              (qmi_lte_disc_set_lted_category_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestPublishMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, nullptr);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestGetDeviceCapabilityMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestGetDeviceCapabilityMessage(
    std::shared_ptr<QcRilRequestGetDeviceCapabilityMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_GET_SUBSCRIPTION_INFO_REQ_V01, NULL, 0,
        sizeof(qmi_lte_get_subscription_info_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          std::shared_ptr<qcril::interfaces::lte_direct::DeviceCapability> devCap = nullptr;
          qmi_lte_get_subscription_info_resp_msg_v01 *response =
              (qmi_lte_get_subscription_info_resp_msg_v01 *)(resp_c_struct.get());

          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
            QCRIL_LOG_INFO("has_capability = %d, capability = %d",
                           response->LteDiscCapability_valid, response->LteDiscCapability);
            if (response->LteDiscCapability_valid) {
              devCap = std::make_shared<qcril::interfaces::lte_direct::DeviceCapability>();
              if (devCap) {
                devCap->setCapability(response->LteDiscCapability);
              }
            }
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestGetDeviceCapabilityMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, devCap);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestGetServiceStatusMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestGetServiceStatusMessage(
    std::shared_ptr<QcRilRequestGetServiceStatusMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_GET_SERVICE_STATUS_REQ_V01, NULL, 0,
        sizeof(qmi_lte_disc_get_service_status_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          std::shared_ptr<qcril::interfaces::lte_direct::ServiceStatus> svcStatus = nullptr;
          qmi_lte_disc_get_service_status_resp_msg_v01 *response =
              (qmi_lte_disc_get_service_status_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
            svcStatus = std::make_shared<qcril::interfaces::lte_direct::ServiceStatus>();
            if (svcStatus) {
              if (response->PublishAllowed_valid) {
                svcStatus->setPublishAllowed(response->PublishAllowed);
              }
              if (response->SubscribeAllowed_valid) {
                svcStatus->setSubscribeAllowed(response->SubscribeAllowed);
              }
            }
            QCRIL_LOG_INFO("SUCCESS: PublishAllowed_valid = %d, PublishAllowed = %d "
                           "SubscribeAllowed_valid = %d, SubscribeAllowed = %d",
                           response->SubscribeAllowed_valid, response->SubscribeAllowed,
                           response->PublishAllowed_valid, response->PublishAllowed);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestGetServiceStatusMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, svcStatus);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestPublishMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestPublishMessage(
    std::shared_ptr<QcRilRequestPublishMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  qmi_lte_disc_publish_req_msg_v01 req = {};
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (msg->hasOsAppId() && !msg->getOsAppId().empty()) {
      if (msg->getOsAppId().size() > QMI_LTE_DISC_OS_APP_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid OS App ID length: %d", msg->getOsAppId().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      memcpy(&req.OsAppId, msg->getOsAppId().c_str(), msg->getOsAppId().size());
    }
    if (msg->hasExpression() && !msg->getExpression().empty()) {
      if (msg->getExpression().size() > QMI_LTE_DISC_PA_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid Expression length: %d", msg->getExpression().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      req.Expression_valid = TRUE;
      memcpy(&req.Expression.prose_app_id_name, msg->getExpression().c_str(),
             msg->getExpression().size());
    }
    if (msg->hasExpressionValidityTime()) {
      req.ExpressionValidityTime_valid = TRUE;
      req.ExpressionValidityTime = msg->getExpressionValidityTime();
    }
    if (msg->hasMetaData() && !msg->getMetaData().empty()) {
      if (msg->getMetaData().size() > QMI_LTE_DISC_PA_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid MetaData length: %d", msg->getMetaData().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      req.MetaData_valid = TRUE;
      memcpy(&req.MetaData, msg->getMetaData().c_str(), msg->getMetaData().size());
    }
    if (msg->hasDiscoveryType()) {
      req.DiscoveryType_valid =
          mapRilDiscoveryTypeToQmi(msg->getDiscoveryType(), &req.DiscoveryType);
    }
    if (msg->hasDuration()) {
      req.PublishDuration_valid = TRUE;
      req.PublishDuration = msg->getDuration();
    }

    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_PUBLISH_REQ_V01, &req, sizeof(req), sizeof(qmi_lte_disc_publish_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          qmi_lte_disc_publish_resp_msg_v01 *response =
              (qmi_lte_disc_publish_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestPublishMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, nullptr);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
          if (rilReqRes == RIL_E_SUCCESS) {
            QCRIL_LOG_INFO("OsAppId_valid = %d, OsAppId = %s", response->OsAppId_valid,
                           response->OsAppId_valid ? response->OsAppId : "<not valid>");

            QCRIL_LOG_INFO("Expression_valid = %d, Expression = %s", response->Expression_valid,
                           response->Expression_valid ? response->Expression.prose_app_id_name
                                                      : "<not valid>");

            QCRIL_LOG_INFO("ExpressionResult_valid = %d, ExpressionResult = %d",
                           response->ExpressionResult_valid,
                           response->ExpressionResult_valid ? response->ExpressionResult : -1);

            if (response->OsAppId_valid && response->Expression_valid &&
                response->ExpressionResult_valid) {
              sendUnsolExpressionStatus(response->OsAppId, &(response->Expression),
                                        response->ExpressionResult);
            }
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  } else {
    qcril_qmi_lte_direct_disc_add_to_exec_overview(msg->getOsAppId().c_str(),
                                                   msg->getExpression().c_str(),
                                                   QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestCancelPublishMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestCancelPublishMessage(
    std::shared_ptr<QcRilRequestCancelPublishMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  qmi_lte_disc_cancel_publish_req_msg_v01 req = {};
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (msg->hasOsAppId() && !msg->getOsAppId().empty()) {
      if (msg->getOsAppId().size() > QMI_LTE_DISC_OS_APP_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid OS App ID length: %d", msg->getOsAppId().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      memcpy(&req.OsAppId, msg->getOsAppId().c_str(), msg->getOsAppId().size());
    }
    if (msg->hasExpression() && !msg->getExpression().empty()) {
      if (msg->getExpression().size() > QMI_LTE_DISC_PA_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid Expression length: %d", msg->getExpression().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      req.Expression_valid = TRUE;
      memcpy(&req.Expression.prose_app_id_name, msg->getExpression().c_str(),
             msg->getExpression().size());
    }
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_CANCEL_PUBLISH_REQ_V01, &req, sizeof(req),
        sizeof(qmi_lte_disc_cancel_publish_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          qmi_lte_disc_cancel_publish_resp_msg_v01 *response =
              (qmi_lte_disc_cancel_publish_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestCancelPublishMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, nullptr);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  } else {
    qcril_qmi_lte_direct_disc_remove_from_exec_overview(msg->getOsAppId().c_str(),
                                                        msg->getExpression().c_str(),
                                                        QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestSubscribeMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestSubscribeMessage(
    std::shared_ptr<QcRilRequestSubscribeMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  qmi_lte_disc_subscribe_req_msg_v01 req = {};
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (msg->hasOsAppId() && !msg->getOsAppId().empty()) {
      if (msg->getOsAppId().size() > QMI_LTE_DISC_OS_APP_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid OS App ID length: %d", msg->getOsAppId().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      memcpy(&req.OsAppId, msg->getOsAppId().c_str(), msg->getOsAppId().size());
    }
    if (msg->hasExpression() && !msg->getExpression().empty()) {
      if (msg->getExpression().size() > QMI_LTE_DISC_PA_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid Expression length: %d", msg->getExpression().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      req.Expression_valid = TRUE;
      memcpy(&req.Expression.prose_app_id_name, msg->getExpression().c_str(),
             msg->getExpression().size());
    }
    if (msg->hasExpressionValidityTime()) {
      req.ExpressionValidityTime_valid = TRUE;
      req.ExpressionValidityTime = msg->getExpressionValidityTime();
    }
    if (msg->hasDiscoveryType()) {
      req.DiscoveryType_valid =
          mapRilDiscoveryTypeToQmi(msg->getDiscoveryType(), &req.DiscoveryType);
    }
    if (msg->hasDuration()) {
      req.SubscribeDuration_valid = TRUE;
      req.SubscribeDuration = msg->getDuration();
    }
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_SUBSCRIBE_REQ_V01, &req, sizeof(req),
        sizeof(qmi_lte_disc_subscribe_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          qmi_lte_disc_subscribe_resp_msg_v01 *response =
              (qmi_lte_disc_subscribe_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestSubscribeMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, nullptr);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
          if (rilReqRes == RIL_E_SUCCESS) {
            QCRIL_LOG_INFO("OsAppId_valid = %d, OsAppId = %s", response->OsAppId_valid,
                           response->OsAppId_valid ? response->OsAppId : "<not valid>");

            QCRIL_LOG_INFO("Expression_valid = %d, Expression = %s", response->Expression_valid,
                           response->Expression_valid ? response->Expression.prose_app_id_name
                                                      : "<not valid>");

            QCRIL_LOG_INFO("ExpressionResult_valid = %d, ExpressionResult = %d",
                           response->ExpressionResult_valid,
                           response->ExpressionResult_valid ? response->ExpressionResult : -1);

            if (response->OsAppId_valid && response->Expression_valid &&
                response->ExpressionResult_valid) {
              sendUnsolExpressionStatus(response->OsAppId, &(response->Expression),
                                        response->ExpressionResult);
            }
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  } else {
    qcril_qmi_lte_direct_disc_add_to_exec_overview(msg->getOsAppId().c_str(),
                                                   msg->getExpression().c_str(),
                                                   QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestCancelSubscribeMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestCancelSubscribeMessage(
    std::shared_ptr<QcRilRequestCancelSubscribeMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  qmi_lte_disc_cancel_subscribe_req_msg_v01 req = {};
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (msg->hasOsAppId() && !msg->getOsAppId().empty()) {
      if (msg->getOsAppId().size() > QMI_LTE_DISC_OS_APP_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid OS App ID length: %d", msg->getOsAppId().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      memcpy(&req.OsAppId, msg->getOsAppId().c_str(), msg->getOsAppId().size());
    }
    if (msg->hasExpression() && !msg->getExpression().empty()) {
      if (msg->getExpression().size() > QMI_LTE_DISC_PA_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid Expression length: %d", msg->getExpression().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      req.Expression_valid = TRUE;
      memcpy(&req.Expression.prose_app_id_name, msg->getExpression().c_str(),
             msg->getExpression().size());
    }
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_CANCEL_SUBSCRIBE_REQ_V01, &req, sizeof(req),
        sizeof(qmi_lte_disc_cancel_subscribe_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          qmi_lte_disc_cancel_subscribe_resp_msg_v01 *response =
              (qmi_lte_disc_cancel_subscribe_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestCancelSubscribeMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, nullptr);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  } else {
    qcril_qmi_lte_direct_disc_remove_from_exec_overview(msg->getOsAppId().c_str(),
                                                        msg->getExpression().c_str(),
                                                        QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// handleQcRilRequestTerminateMessage
//===========================================================================
void LteDirectModule::handleQcRilRequestTerminateMessage(
    std::shared_ptr<QcRilRequestTerminateMessage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  qmi_lte_disc_terminate_req_msg_v01 req = {};
  RIL_Errno rilErr = RIL_E_SUCCESS;
  auto pendingMsgStatus = std::make_pair(0, false);

  do {
    if (msg->hasOsAppId() && !msg->getOsAppId().empty()) {
      if (msg->getOsAppId().size() > QMI_LTE_DISC_OS_APP_ID_NAME_MAX_V01) {
        QCRIL_LOG_ERROR("Invalid OS App ID length: %d", msg->getOsAppId().size());
        rilErr = RIL_E_GENERIC_FAILURE;
        break;
      }
      memcpy(&req.OsAppId, msg->getOsAppId().c_str(), msg->getOsAppId().size());
    }
    pendingMsgStatus = getPendingMessageList().insert(msg);
    if (pendingMsgStatus.second != true) {
      QCRIL_LOG_ERROR("getPendingMessageList().insert failed!!");
      rilErr = RIL_E_GENERIC_FAILURE;
      break;
    }
    uint32 user_data = QCRIL_COMPOSE_USER_DATA(QCRIL_DEFAULT_INSTANCE_ID, QCRIL_DEFAULT_MODEM_ID,
                                               pendingMsgStatus.first);
    qmi_client_error_type clientErr = qmi_client_lte_send_async(
        QMI_LTE_DISC_TERMINATE_REQ_V01, &req, sizeof(req),
        sizeof(qmi_lte_disc_terminate_resp_msg_v01),
        [this](unsigned int /*msg_id*/, std::shared_ptr<void> resp_c_struct,
               unsigned int /*resp_c_struct_len*/, void *resp_cb_data,
               qmi_client_error_type transp_err) -> void {
          RIL_Errno rilReqRes = RIL_E_GENERIC_FAILURE;
          uint32 user_data = (uint32)(uintptr_t)resp_cb_data;
          uint16_t pendingReqId = QCRIL_EXTRACT_USER_ID_FROM_USER_DATA(user_data);
          qmi_lte_disc_terminate_resp_msg_v01 *response =
              (qmi_lte_disc_terminate_resp_msg_v01 *)(resp_c_struct.get());
          if (transp_err == QMI_NO_ERR && response) {
            rilReqRes = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(QMI_NO_ERR,
                                                                                &response->resp);
          }
          auto pendingMsg = getPendingMessageList().extract(pendingReqId);
          if (pendingMsg) {
            auto msg(std::static_pointer_cast<QcRilRequestTerminateMessage>(pendingMsg));
            auto respPayload =
                std::make_shared<QcRilRequestMessageCallbackPayload>(rilReqRes, nullptr);
            msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
          }
        },
        (void *)(uintptr_t)user_data);
    rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
    QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);
  } while (FALSE);

  if (rilErr != RIL_E_SUCCESS) {
    if (pendingMsgStatus.second) {
      getPendingMessageList().erase(pendingMsgStatus.first);
    }
    auto respPayload = std::make_shared<QcRilRequestMessageCallbackPayload>(rilErr, nullptr);
    msg->sendResponse(msg, Message::Callback::Status::SUCCESS, respPayload);
  } else {
    qcril_qmi_lte_direct_disc_remove_from_exec_overview(msg->getOsAppId().c_str(), NULL,
                                                        QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH |
                                                            QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE);
  }
  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// qcril_qmi_lte_direct_disc_ind_registrations
//===========================================================================
static void qcril_qmi_lte_direct_disc_ind_registrations(void) {
  qmi_lte_indication_reg_req_msg_v01 indication_req = {};

  QCRIL_LOG_FUNC_ENTRY();

  indication_req.indication_bitmask =
      (QMI_LTE_DISC_NOTIFICATION_IND_MASK_V01 | QMI_LTE_DISC_BROADCAST_NOTIFICATION_IND_MASK_V01 |
       QMI_LTE_DISC_MATCH_NOTIFICATION_IND_MASK_V01 | QMI_LTE_DISC_PSK_EXPIRED_IND_MASK_V01);

  qmi_client_error_type clientErr = qmi_client_lte_send_async(
      QMI_LTE_INDICATION_REGISTER_REQ_V01, &indication_req, sizeof(indication_req),
      sizeof(qmi_lte_indication_reg_resp_msg_v01),
      [](unsigned int /*msg_id*/, std::shared_ptr<void> /*resp_c_struct*/,
         unsigned int /*resp_c_struct_len*/, void * /*resp_cb_data*/,
         qmi_client_error_type /*transp_err*/) -> void {},
      nullptr);
  RIL_Errno rilErr = qcril_qmi_util_convert_qmi_response_codes_to_ril_result(clientErr, NULL);
  QCRIL_LOG_ESSENTIAL("rilErr = %d", (int)rilErr);

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// qcril_qmi_lte_direct_disc_init
//===========================================================================
RIL_Errno qcril_qmi_lte_direct_disc_init(void) {
  RIL_Errno res = RIL_E_SUCCESS;

  QCRIL_LOG_FUNC_ENTRY();

  memset(&lte_direct_disc_overview, 0, sizeof(lte_direct_disc_overview));

  qcril_qmi_lte_direct_disc_ind_registrations();

  QCRIL_LOG_FUNC_RETURN_WITH_RET((int)res);

  return res;
}

//===========================================================================
// qcril_qmi_lte_direct_disc_add_to_exec_overview
//===========================================================================
qcril_qmi_lte_direct_disc_exec_overview_type *
qcril_qmi_lte_direct_disc_add_to_exec_overview(const char *os_app_id, const char *exp,
                                               uint32_t op) {
  qcril_qmi_lte_direct_disc_exec_overview_type *entry = NULL;
  qcril_qmi_lte_direct_disc_exec_overview_type *iter = NULL;
  qcril_qmi_lte_direct_disc_overview_exp_type *exp_iter = NULL;
  qcril_qmi_lte_direct_disc_overview_exp_type *exp_entry = NULL;

  QCRIL_LOG_FUNC_ENTRY();

  if (os_app_id && exp) {
    QCRIL_LOG_INFO("Add Os App Id: %s, expression: %s", os_app_id, exp);
    QCRIL_LOG_INFO("lte_direct_disc_overview.exec_overview_root = 0x%x",
                   lte_direct_disc_overview.exec_overview_root);
    iter = lte_direct_disc_overview.exec_overview_root;
    while (iter) {
      QCRIL_LOG_INFO("iter Os App Id: %s", iter->os_app_id);
      if (strcmp(iter->os_app_id, os_app_id) == 0) {
        QCRIL_LOG_INFO("Found Os App Id match");
        entry = iter;
        break;
      }
      iter = iter->next;
    }
    QCRIL_LOG_INFO("entry = 0x%x", entry);

    if (entry) {
      if (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH) {
        QCRIL_LOG_INFO("Look for publish expression");
        exp_iter = entry->publish_list;
        while (exp_iter) {
          QCRIL_LOG_INFO("exp_iter expression: %s", exp_iter->expression);
          if (strcmp(exp_iter->expression, exp) == 0) {
            QCRIL_LOG_INFO("Found publish expression match");
            exp_entry = exp_iter;
            break;
          }
          exp_iter = exp_iter->next;
        }
        if (!exp_entry) {
          QCRIL_LOG_INFO("Create new publish expression and add to list");
          exp_entry = (qcril_qmi_lte_direct_disc_overview_exp_type *)qcril_malloc(
              sizeof(qcril_qmi_lte_direct_disc_overview_exp_type));
          if (exp_entry != nullptr) {
            exp_entry->expression = qmi_ril_util_str_clone((char *)exp);
            exp_entry->next = entry->publish_list;
            entry->publish_list = exp_entry;
          }
        } else {
          QCRIL_LOG_INFO("expression is already present in the publish list");
        }
      }
      if (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE) {
        QCRIL_LOG_INFO("Look for subscribe expression");
        exp_iter = entry->subscribe_list;
        while (exp_iter) {
          QCRIL_LOG_INFO("exp_iter expression: %s", exp_iter->expression);
          if (strcmp(exp_iter->expression, exp) == 0) {
            QCRIL_LOG_INFO("Found subscribe expression match");
            exp_entry = exp_iter;
            break;
          }
          exp_iter = exp_iter->next;
        }
        if (!exp_entry) {
          QCRIL_LOG_INFO("Create new subscribe expression and add to list");
          exp_entry = (qcril_qmi_lte_direct_disc_overview_exp_type *)qcril_malloc(
              sizeof(qcril_qmi_lte_direct_disc_overview_exp_type));
          if (exp_entry != nullptr) {
            exp_entry->expression = qmi_ril_util_str_clone((char *)exp);
            exp_entry->next = entry->subscribe_list;
            entry->subscribe_list = exp_entry;
          }
        } else {
          QCRIL_LOG_INFO("expression is already present in the subscribe list");
        }
      }
    } else {
      QCRIL_LOG_INFO("Add new entry for Os App Id: %s", os_app_id);

      // Create expression list entry (publish/suscribe)
      exp_entry = (qcril_qmi_lte_direct_disc_overview_exp_type *)qcril_malloc(
          sizeof(qcril_qmi_lte_direct_disc_overview_exp_type));
      if (exp_entry != nullptr) {
        exp_entry->expression = qmi_ril_util_str_clone((char *)exp);
        exp_entry->next = NULL;

        // Create os_app_id entry
        entry = qcril_malloc2(entry);
        if (entry != nullptr) {
          entry->os_app_id = qmi_ril_util_str_clone((char *)os_app_id);
          if (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH) {
            entry->publish_list = exp_entry;
          } else if (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE) {
            entry->subscribe_list = exp_entry;
          }
          entry->next = lte_direct_disc_overview.exec_overview_root;
          lte_direct_disc_overview.exec_overview_root = entry;
        }
      }
    }
  }

  QCRIL_LOG_FUNC_RETURN();

  return entry;
}

//===========================================================================
// qcril_qmi_lte_direct_disc_remove_from_exec_overview
//===========================================================================
void qcril_qmi_lte_direct_disc_remove_from_exec_overview(const char *os_app_id, const char *exp,
                                                         uint32_t op) {
  qcril_qmi_lte_direct_disc_exec_overview_type *overview_prev = NULL;
  qcril_qmi_lte_direct_disc_exec_overview_type *overview_iter = NULL;

  qcril_qmi_lte_direct_disc_overview_exp_type *exp_iter = NULL;
  qcril_qmi_lte_direct_disc_overview_exp_type *exp_prev = NULL;
  qcril_qmi_lte_direct_disc_overview_exp_type *exp_entry = NULL;

  QCRIL_LOG_FUNC_ENTRY();

  if (os_app_id != NULL) {
    QCRIL_LOG_INFO("Remove req for Os App Id: %s, expression: %s", os_app_id, exp);
    overview_prev = NULL;
    overview_iter = lte_direct_disc_overview.exec_overview_root;
    while (overview_iter) {
      if (strcmp(overview_iter->os_app_id, os_app_id) == 0) {
        QCRIL_LOG_INFO("Found Os App Id match");
        break;
      }
      overview_prev = overview_iter;
      overview_iter = overview_iter->next;
    }
  }

  if (overview_iter) {
    if (exp) {
      if (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH) {
        QCRIL_LOG_INFO("Look for publish expression");
        exp_iter = overview_iter->publish_list;
        exp_prev = NULL;
        while (exp_iter) {
          if (strcmp(exp_iter->expression, exp) == 0) {
            QCRIL_LOG_INFO("Found publish expression match");
            break;
          }
          exp_prev = exp_iter;
          exp_iter = exp_iter->next;
        }
        if (exp_iter) {
          QCRIL_LOG_INFO("Remove matched publish expression : 0x%x", exp_iter);
          if (exp_prev == NULL) {
            overview_iter->publish_list = exp_iter->next;
          } else {
            exp_prev->next = exp_iter->next;
          }
          qcril_free(exp_iter->expression);
          qcril_free(exp_iter);
        }
      }
      if (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE) {
        QCRIL_LOG_INFO("Look for subscribe expression");
        exp_iter = overview_iter->subscribe_list;
        exp_prev = NULL;
        while (exp_iter) {
          if (strcmp(exp_iter->expression, exp) == 0) {
            QCRIL_LOG_INFO("Found subscribe expression match");
            break;
          }
          exp_prev = exp_iter;
          exp_iter = exp_iter->next;
        }
        if (exp_iter) {
          QCRIL_LOG_INFO("Remove matched subscribe expression : 0x%x", exp_iter);
          if (exp_prev == NULL) {
            overview_iter->subscribe_list = exp_iter->next;
          } else {
            exp_prev->next = exp_iter->next;
          }
          qcril_free(exp_iter->expression);
          qcril_free(exp_iter);
        }
      }
    } else {
      QCRIL_LOG_INFO("Remove all expression matching OP: 0x%x", op);
      // Delete all publish/subscribe list
      while (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH && overview_iter->publish_list) {
        exp_entry = overview_iter->publish_list;
        overview_iter->publish_list = exp_entry->next;
        qcril_free(exp_entry->expression);
        qcril_free(exp_entry);
      }
      while (op & QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE && overview_iter->subscribe_list) {
        exp_entry = overview_iter->subscribe_list;
        overview_iter->subscribe_list = exp_entry->next;
        qcril_free(exp_entry->expression);
        qcril_free(exp_entry);
      }
    }

    if (overview_iter->publish_list == NULL && overview_iter->subscribe_list == NULL) {
      QCRIL_LOG_INFO("No publish/subscribe list, remove overview entry");
      if (overview_prev == NULL) {
        lte_direct_disc_overview.exec_overview_root = overview_iter->next;
      } else {
        overview_prev->next = overview_iter->next;
      }
      qcril_free(overview_iter->os_app_id);
      qcril_free(overview_iter);
    }
  }

  QCRIL_LOG_FUNC_RETURN();
}

//===========================================================================
// qcril_qmi_lte_direct_disc_terminate_all_apps
//===========================================================================
void qcril_qmi_lte_direct_disc_terminate_all_apps() {
  qcril_qmi_lte_direct_disc_exec_overview_type *entry = NULL;
  qcril_qmi_lte_direct_disc_exec_overview_type *iter = NULL;
  qcril_qmi_lte_direct_disc_overview_exp_type *exp_entry = NULL;

  qmi_lte_disc_terminate_req_msg_v01 req;

  QCRIL_LOG_FUNC_ENTRY();

  entry = NULL;
  iter = lte_direct_disc_overview.exec_overview_root;
  while (iter) {
    entry = iter;
    iter = iter->next;

    memset(&req, 0x00, sizeof(req));
    memcpy(&req.OsAppId, entry->os_app_id, strlen(entry->os_app_id));

    QCRIL_LOG_INFO("Terminate Os App Id: %s", entry->os_app_id);

    qmi_client_lte_send_async(
        QMI_LTE_DISC_TERMINATE_REQ_V01, &req, sizeof(req),
        sizeof(qmi_lte_disc_terminate_resp_msg_v01),
        [](unsigned int /*msg_id*/, std::shared_ptr<void> /*resp_c_struct*/,
           unsigned int /*resp_c_struct_len*/, void * /*resp_cb_data*/,
           qmi_client_error_type /*transp_err*/) -> void {},
        nullptr);

    while (entry->publish_list) {
      exp_entry = entry->publish_list;
      entry->publish_list = exp_entry->next;
      qcril_free(exp_entry->expression);
      qcril_free(exp_entry);
    }
    while (entry->subscribe_list) {
      exp_entry = entry->subscribe_list;
      entry->subscribe_list = exp_entry->next;
      qcril_free(exp_entry->expression);
      qcril_free(exp_entry);
    }
    qcril_free(entry->os_app_id);
    qcril_free(entry);
  }

  lte_direct_disc_overview.exec_overview_root = NULL;

  QCRIL_LOG_FUNC_RETURN();
}

#ifdef QMI_RIL_UTF
//===========================================================================
// cleanup
//===========================================================================
void LteDirectModule::cleanup() {
  Log::getInstance().d("[" + mName + "]: cleanup");
  std::shared_ptr<LteModemEndPoint> mLteModemEndPoint =
      ModemEndPointFactory<LteModemEndPoint>::getInstance().buildEndPoint();
  LteModemEndPointModule *mLteModemEndPointModule =
      (LteModemEndPointModule *)mLteModemEndPoint->mModule;

  mLteModemEndPointModule->cleanUpQmiSvcClient();
  mReady = false;
  // qcril_qmi_lte_pre_init();
}

//===========================================================================
// qcril_qmi_hal_lte_direct_module_cleanup
//===========================================================================
void qcril_qmi_hal_lte_direct_module_cleanup() { getLteDirectModule()->cleanup(); }
#endif
