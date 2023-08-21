/******************************************************************************
#  Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#include "DSDModemEndPoint.h"
#include "sync/RegisterForSystemStatusSyncMessage.h"
#include "sync/TurnOnAPAssistIWLANSyncMessage.h"
#include "sync/RegisterForAPAsstIWlanIndsSyncMessage.h"
#include "sync/RegisterForRoamingStatusChangedMessage.h"
#include "sync/TriggerDDSSwitchSyncMessage.h"
#include "sync/SetCapabilitiesMessage.h"
#include "request/GetDsdSystemStatusMessage.h"
#include "request/GetDataNrIconTypeMessage.h"
#include "UnSolMessages/DDSSwitchResultIndMessage.h"
#include "UnSolMessages/DsdSystemStatusMessage.h"
#include "UnSolMessages/DsdSystemStatusPerApnMessage.h"
#include "UnSolMessages/DataNrIconTypeIndMessage.h"
#include "MessageCommon.h"
#include <sstream>

using std::to_string;
using namespace rildata;
constexpr const char *DSDModemEndPoint::NAME;

void DSDModemEndPoint::requestSetup(string clientToken,
                                      GenericCallback<string>* callback)
{
  auto shared_setupMsg = std::make_shared<QmiSetupRequest>
        (clientToken, 0, nullptr, callback);
  mModule->dispatch(shared_setupMsg);
}

boolean DSDModemEndPoint::isApnTypeFound
(
  const RIL_ApnTypes &apn_type, const uint32_t &apn_types_mask
)
{
  if ((apn_type & apn_types_mask) == apn_type)
  {
    Log::getInstance().d("apn type %x was enabled" + std::to_string(apn_type));
    return TRUE;
  }
  return FALSE;
}

void DSDModemEndPoint::sendApnInfoQmi
(
  const std::string &apn_name,
  dsd_apn_type_enum_v01 apn_type
)
{
  Log::getInstance().d("DSDModemEndPoint::sendApnInfoQmi ENTRY");
  dsd_set_apn_info_req_msg_v01 qmiReq;
  dsd_set_apn_info_resp_msg_v01 qmiResp;

  memset(&qmiReq, 0, sizeof(qmiReq));
  memset(&qmiResp, 0, sizeof(qmiResp));
  strlcpy(&qmiReq.apn_info.apn_name[0], apn_name.c_str(), QMI_DSD_MAX_APN_LEN_V01);

  // indicate that the apn_invalid flag is set
  qmiReq.apn_invalid_valid = FALSE;
  qmiReq.apn_info.apn_type = apn_type;

  auto retVal = sendRawSync(QMI_DSD_SET_APN_INFO_REQ_V01,
                            (void *)&qmiReq, sizeof(qmiReq),
                            (void *)&qmiResp, sizeof(qmiResp),
                            QCRIL_DATA_QMI_TIMEOUT);
  if (retVal != QMI_NO_ERR) {
    Log::getInstance().d("[DSDModemEndPoint] Failed to send"
      "QMI_DSD_SET_APN_INFO_REQ_V01 for apn_name:"+apn_name+
      ", type:"+std::to_string(apn_type)+"with rc ="+std::to_string(retVal));
  } else {
    Log::getInstance().d("[DSDModemEndPoint] sendApnInfoQmi::Successfully sent"
      "QMI_DSD_SET_APN_INFO_REQ_V01 for type ="+ std::to_string(apn_type));
  }
}

Message::Callback::Status DSDModemEndPoint::setApnInfoSync
( const std::string apn_name,
  const uint32_t apn_type_mask)
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::handleSetApnInfo]"
                         "dispatching message SetApnInfoSyncMessage");

    Log::getInstance().d("[DSDModemEndPoint]: apn_name= " +apn_name+"apn_types= %d" +std::to_string(apn_type_mask));
    do
    {
      if (apn_type_mask <= 0)
      {
        Log::getInstance().d("Invalid inputs");
        return Message::Callback::Status::SUCCESS;
      }

      if (isApnTypeFound(RIL_APN_TYPE_ALL, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_DEFAULT_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_IMS_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_MMS_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_DUN_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_SUPL_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_HIPRI_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_FOTA_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_CBS_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_IA_V01);
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_EMERGENCY_V01);
        break;
      }
      if (isApnTypeFound(RIL_APN_TYPE_DEFAULT, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_DEFAULT_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_IMS, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_IMS_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_MMS, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_MMS_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_DUN, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_DUN_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_SUPL, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_SUPL_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_HIPRI, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_HIPRI_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_FOTA, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_FOTA_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_CBS, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_CBS_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_IA, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_IA_V01);
      }
      if (isApnTypeFound(RIL_APN_TYPE_EMERGENCY, apn_type_mask))
      {
        sendApnInfoQmi(apn_name, DSD_APN_TYPE_EMERGENCY_V01);
      }
    } while(0);
    return Message::Callback::Status::SUCCESS;
  } else {
    Log::getInstance().d("[DSDModemEndPoint::handleSetApnInfo]"
                         "Failed to send message SetApnInfoSyncMessage");
    return Message::Callback::Status::FAILURE;
  }
}

Message::Callback::Status DSDModemEndPoint::sendAPAssistIWLANSupportedSync ( )
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::sendAPAssistIWLANSupportedSync]");
    // Note that callback is not required for sync calls.
    auto msg = std::make_shared<TurnOnAPAssistIWLANSyncMessage>(nullptr);
    Message::Callback::Status apiStatus;
    auto r = std::make_shared<bool>();
    apiStatus = msg->dispatchSync(r);
    std::ostringstream ss;
    ss << "[DSDModemEndPoint::sendAPAssistIWLANSupportedSync] status = " << (int) apiStatus;
    Log::getInstance().d(ss.str());
    return apiStatus;
  } else {
    Log::getInstance().d("[DSDModemEndPoint::sendAPAssistIWLANSupportedSync]"
                         "Failed to send message TurnOnAPAssistIWLANSyncMessage");
    return Message::Callback::Status::FAILURE;
  }
}

void DSDModemEndPoint::generateDsdSystemStatusInd()
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::generateDsdSystemStatusInd]"
                         "dispatching message GetDsdSystemStatusMessage");
    auto msg = std::make_shared<GetDsdSystemStatusMessage>();
    GenericCallback<DsdSystemStatusResult_t> cb([](std::shared_ptr<Message>, Message::Callback::Status status,
                            std::shared_ptr<DsdSystemStatusResult_t> r) -> void {
      if (status == Message::Callback::Status::SUCCESS && r != nullptr) {
        auto indMsg = std::make_shared<rildata::DsdSystemStatusMessage>(r->resp_ind);
        indMsg->broadcast();
        auto perApnIndMsg = std::make_shared<rildata::DsdSystemStatusPerApnMessage>(r->apn_sys);
        perApnIndMsg->broadcast();
      } else {
        Log::getInstance().d("[DSDModemEndPoint::generateDsdSystemStatusInd] Failed to get system status");
      }
    });
    msg->setCallback(&cb);
    msg->dispatch();
  } else {
    Log::getInstance().d("[DSDModemEndPoint::generateDsdSystemStatusInd]"
                         "Failed to send message GetDsdSystemStatusMessage");
  }
}

Message::Callback::Status DSDModemEndPoint::registerForSystemStatusSync ( )
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::registerForSystemStatusSync]"
                         "dispatching message RegisterForSystemStatusSyncMessage");
    // Note that callback is not required for sync calls.
    auto msg = std::make_shared<RegisterForSystemStatusSyncMessage>(nullptr);
    Message::Callback::Status apiStatus;
    auto r = std::make_shared<int>();
    apiStatus = msg->dispatchSync(r);
    std::ostringstream ss;
    ss << "[DSDModemEndPoint::registerForSystemStatusSync] status = " << (int) apiStatus;
    Log::getInstance().d(ss.str());
    return apiStatus;
  } else {
    Log::getInstance().d("[DSDModemEndPoint::registerForSystemStatusSync]"
                         "Failed to send message RegisterForSystemStatusSyncMessage");
    return Message::Callback::Status::FAILURE;
  }
}

Message::Callback::Status DSDModemEndPoint::setV2Capabilities() {
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    auto msg = std::make_shared<SetCapabilitiesMessage>();
    Message::Callback::Status apiStatus;
    auto r = std::make_shared<int>();
    msg->setSystemStatusV2(true);
    msg->setUiInfoV2(true);
    apiStatus = msg->dispatchSync(r);
    std::ostringstream ss;
    ss << "[DSDModemEndPoint::setV2Capabilities] status = " << (int) apiStatus;
    Log::getInstance().d(ss.str());
    return apiStatus;
  }

  Log::getInstance().d("[DSDModemEndPoint::setV2Capabilities]"
                       "Failed to send message SetCapabilitiesMessage");
  return Message::Callback::Status::FAILURE;
}

Message::Callback::Status DSDModemEndPoint::registerForCurrentDDSInd ( )
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    dsd_indication_register_req_msg_v01 ind_req;
    dsd_indication_register_resp_msg_v01 ind_resp;
    memset(&ind_req, 0, sizeof(ind_req));
    memset(&ind_resp, 0, sizeof(ind_resp));

    ind_req.report_current_dds_valid = true;
    ind_req.report_current_dds = 1;

    int rc = sendRawSync(QMI_DSD_INDICATION_REGISTER_REQ_V01,
                         (void *)&ind_req, sizeof(ind_req),
                         (void *)&ind_resp, sizeof(ind_resp),
                         QCRIL_DATA_QMI_TIMEOUT);
    if ((rc != QMI_NO_ERR) || (ind_resp.resp.result == QMI_RESULT_FAILURE_V01 )) {
      Log::getInstance().d("[DSDModemEndPointModule] Failed to send QMI_DSD_INDICATION_REGISTER_REQ_V01");
    } else {
      Log::getInstance().d("[DSDModemEndPointModule] QMI_DSD_INDICATION_REGISTER_REQ_V01::Successfully sent");
      return Message::Callback::Status::SUCCESS;
    }
  } else {
    Log::getInstance().d("[DSDModemEndPoint::registerForCurrentDDSInd]"
                         "Failed to send message registerForCurrentDDSInd");
  }
  return Message::Callback::Status::FAILURE;
}

Message::Callback::Status DSDModemEndPoint::registerForAPAsstIWlanIndsSync (bool toRegister )
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::registerForAPAsstIWlanIndsSync]"
                         "dispatching message RegisterForAPAsstIWlanIndsSyncMessage register "
                         + std::to_string(toRegister));
    // Note that callback is not required for sync calls.
    auto msg = std::make_shared<RegisterForAPAsstIWlanIndsSyncMessage>(nullptr);
    Message::Callback::Status apiStatus;
    auto r = std::make_shared<int>();
    msg->setParams(toRegister);
    apiStatus = msg->dispatchSync(r);
    std::ostringstream ss;
    ss << "[DSDModemEndPoint::registerForAPAsstIWlanIndsSync] status = " << (int) apiStatus;
    Log::getInstance().d(ss.str());
    return apiStatus;
  } else {
    Log::getInstance().d("[DSDModemEndPoint::registerForAPAsstIWlanIndsSync]"
                         "Failed to send message RegisterForAPAsstIWlanIndsSyncMessage");
    return Message::Callback::Status::FAILURE;
  }
}

Message::Callback::Status DSDModemEndPoint::registerForRoamingStatusChanged ( )
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::registerForRoamingStatusChanged]"
                         "dispatching message RegisterForRoamingStatusChangedMessage");
    // Note that callback is not required for sync calls.
    auto msg = std::make_shared<RegisterForRoamingStatusChangedMessage>(nullptr);
    Message::Callback::Status apiStatus;
    auto r = std::make_shared<int>();
    apiStatus = msg->dispatchSync(r);
    std::ostringstream ss;
    ss << "[DSDModemEndPoint::registerForRoamingStatusChanged] status = " << (int) apiStatus;
    Log::getInstance().d(ss.str());
    return apiStatus;
  } else {
    Log::getInstance().d("[DSDModemEndPoint::registerForRoamingStatusChanged]"
                         "Failed to send message RegisterForRoamingStatusChangedMessage");
    return Message::Callback::Status::FAILURE;
  }
}

Message::Callback::Status DSDModemEndPoint::setApnPreferredSystemChangeSync(
  const std::string apnName, const int32_t prefRat)
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::setApnPreferredSystemChangeSync]"
                         "dispatching message SetApnPreferredSystemSyncMessage");

    dsd_set_apn_preferred_system_req_msg_v01 qmiReq;
    dsd_set_apn_preferred_system_resp_msg_v01 qmiResp;
    memset(&qmiReq, 0, sizeof(qmiReq));
    memset(&qmiResp, 0, sizeof(qmiResp));

    strlcpy(qmiReq.apn_pref_sys.apn_name, apnName.c_str(), QMI_DSD_MAX_APN_LEN_V01+1);
    qmiReq.apn_pref_sys.pref_sys = (dsd_apn_pref_sys_enum_v01)prefRat;
    qmiReq.is_ap_asst_valid = true;
    qmiReq.is_ap_asst = true;
    auto retVal = sendRawSync(QMI_DSD_SET_APN_PREFERRED_SYSTEM_REQ_V01,
                              (void *)&qmiReq, sizeof(qmiReq),
                              (void *)&qmiResp, sizeof(qmiResp),
                              QCRIL_DATA_QMI_TIMEOUT);
    if (retVal != QMI_NO_ERR) {
      Log::getInstance().d("[DSDModemEndPoint] Failed to send"
        "QMI_DSD_SET_APN_PREFERRED_SYSTEM_REQ_V01");
      return Message::Callback::Status::FAILURE;
    } else {
      return Message::Callback::Status::SUCCESS;
    }
  } else {
    Log::getInstance().d("[DSDModemEndPoint::setApnPreferredSystemChangeSync]"
                         "Failed to send message SetApnPreferredSystemSyncMessage");
    return Message::Callback::Status::FAILURE;
  }
}

Message::Callback::Status DSDModemEndPoint::getCurrentDDSSync(DDSSubIdInfo &ddsInfo)
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    dsd_get_current_dds_req_msg_v01 qmiReq;
    dsd_get_current_dds_resp_msg_v01 qmiResp;
    memset(&qmiReq, 0, sizeof(qmiReq));
    memset(&qmiResp, 0, sizeof(qmiResp));
    auto retVal = sendRawSync(QMI_DSD_GET_CURRENT_DDS_REQ_V01,
                              (void *)&qmiReq, sizeof(qmiReq),
                              (void *)&qmiResp, sizeof(qmiResp),
                              QCRIL_DATA_QMI_TIMEOUT);
    if (retVal != QMI_NO_ERR) {
      Log::getInstance().d("[DSDModemEndPoint] Failed to send"
        "QMI_DSD_GET_CURRENT_DDS_REQ_V01");
      return Message::Callback::Status::FAILURE;
    } else if (!qmiResp.dds_valid) {
      // TODO: discuss how to handle case where temporary dds is returned

      Log::getInstance().d("[DSDModemEndPoint] QMI_DSD_GET_CURRENT_DDS_REQ_V01 "
        "did not return dds");
      return Message::Callback::Status::FAILURE;
    }
    switch (qmiResp.dds) {
      case DSD_PRIMARY_SUBS_V01:
        ddsInfo.dds_sub_id = 0;
        break;
      case DSD_SECONDARY_SUBS_V01:
        ddsInfo.dds_sub_id = 1;
        break;
      default:
        Log::getInstance().d("[DSDModemEndPoint] QMI_DSD_GET_CURRENT_DDS_REQ_V01 "
          "invalid subId=" + std::to_string(qmiResp.dds));
        return Message::Callback::Status::FAILURE;
    }
    if(qmiResp.dds_switch_type_valid) {

      switch(qmiResp.dds_switch_type) {
        case DSD_DDS_SWITCH_PERMANENT_V01:
          ddsInfo.switch_type = DSD_DDS_DURATION_PERMANANT_V01;
          break;
        case DSD_DDS_SWITCH_TEMPORARY_V01:
          ddsInfo.switch_type = DSD_DDS_DURATION_TEMPORARY_V01;
          break;
        default:
          ddsInfo.switch_type = DSD_DDS_DURATION_PERMANANT_V01;
      }
    } else {
      ddsInfo.switch_type = DSD_DDS_DURATION_PERMANANT_V01; //default value
    }
    return Message::Callback::Status::SUCCESS;
  } else {
    Log::getInstance().d("[DSDModemEndPoint::getCurrentDDSSync]"
                         "Failed to send message QMI_DSD_GET_CURRENT_DDS_REQ_V01");
    return Message::Callback::Status::FAILURE;
  }
}

Message::Callback::Status DSDModemEndPoint::triggerDDSSwitchSync(int subId, int& error, dsd_dds_switch_type_enum_v01 switch_type)
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    Log::getInstance().d("[DSDModemEndPoint::triggerDDSSwitchSync]"
                         "dispatching message TriggerDDSSwitchSyncMessage");
    auto msg = std::make_shared<TriggerDDSSwitchSyncMessage>(subId, switch_type);
    auto retVal = std::make_shared<SetPreferredDataModemResult_t>();
    auto apiStatus = msg->dispatchSync(retVal);
    Log::getInstance().d("[DSDModemEndPoint::triggerDDSSwitchSync] status=" +
                         std::to_string(static_cast<int>(apiStatus)));
    if (retVal == nullptr) {
      Log::getInstance().d("[DSDModemEndPoint::triggerDDSSwitchSync] returned null");
      error = static_cast<int>(SetPreferredDataModemResult_t::QMI_ERROR);
      return Message::Callback::Status::FAILURE;
    }
    error = static_cast<int>(*retVal);
    Log::getInstance().d("[DSDModemEndPoint::triggerDDSSwitchSync] returned errorcode=" +
                         std::to_string(error));
    return apiStatus;
  } else {
    Log::getInstance().d("[DSDModemEndPoint::triggerDDSSwitchSync]"
                         "Failed to send message TriggerDDSSwitchSyncMessage");
    error = static_cast<int>(SetPreferredDataModemResult_t::MODEM_ENDPOINT_NOT_OPERATIONAL);
    auto msg = std::make_shared<DDSSwitchResultIndMessage>();
    msg->setError(TriggerDDSSwitchError::MODEM_NOT_UP);
    msg->broadcast();
    return Message::Callback::Status::FAILURE;
  }
}

int DSDModemEndPoint::toggleLimitedSysIndicationChangeReport(bool report)
{
  Log::getInstance().d("[DSDModemEndPoint]: toggleLimitedSysIndicationChangeReport "+std::to_string((int)report));
  if (mReportLimitedSysIndicationChange != report) {
    mReportLimitedSysIndicationChange = report;

    dsd_system_status_change_req_msg_v01  qmiReq;
    dsd_system_status_change_resp_msg_v01 qmiResp;
    memset(&qmiReq, 0, sizeof(qmiReq));
    memset(&qmiResp, 0, sizeof(qmiResp));

    qmiReq.limit_so_mask_change_ind_valid = true;
    qmiReq.limit_so_mask_change_ind = report;

    auto retVal = sendRawSync(QMI_DSD_SYSTEM_STATUS_CHANGE_REQ_V01,
                          (void *)&qmiReq, sizeof(qmiReq),
                          (void *)&qmiResp, sizeof(qmiResp),
                          QCRIL_DATA_QMI_TIMEOUT);
    if (retVal != QMI_NO_ERR) {
      Log::getInstance().d("[DSDModemEndPoint::toggleLimitedSysIndicationChangeReport] Failed to register");
    }

    /* when mReportLimitedSysIndicationChange = true, limit the DsdSystemStatusMessage*/
    if (!mReportLimitedSysIndicationChange) {
      dsd_get_system_status_resp_msg_v01  sysRespMsg;
      dsd_system_status_ind_msg_v01 *ind_data = nullptr;
      memset(&sysRespMsg, 0, sizeof(sysRespMsg));
      retVal = sendRawSync(QMI_DSD_GET_SYSTEM_STATUS_REQ_V01,
                            nullptr, 0,
                            (void *)&sysRespMsg, sizeof(sysRespMsg),
                            QCRIL_DATA_QMI_TIMEOUT);
      if (retVal == QMI_NO_ERR) {
        Log::getInstance().d("[DSDModemEndPoint::toggleLimitedSysIndicationChangeReport] success to query");
        ind_data = (dsd_system_status_ind_msg_v01 *)((char *)&sysRespMsg +
                                                    offsetof(dsd_get_system_status_resp_msg_v01,
                                                    avail_sys_valid));

        auto indMsg = std::make_shared<rildata::DsdSystemStatusMessage>(*ind_data);
        indMsg->broadcast();
      }
      else {
        Log::getInstance().d("[DSDModemEndPoint::toggleLimitedSysIndicationChangeReport] Failed to query");
      }
    }
  }

  return 0;
}

Message::Callback::Status DSDModemEndPoint::handleScreenStateChangeInd(bool screenEnabled)
{
  dsd_register_ui_info_change_req_msg_v01 qmiReq;
  memset(&qmiReq, 0, sizeof(qmiReq));

  if( ((screenEnabled == true ) && ( mScreenState == ScreenState_t::SCREEN_ON )) ||
      ((screenEnabled == false ) && ( mScreenState == ScreenState_t::SCREEN_OFF )))
  {
    Log::getInstance().d("[DSDModemEndPoint::No change in Screen State!!");
    return Message::Callback::Status::SUCCESS;
  } else if ((screenEnabled == false ) && ( mScreenState == ScreenState_t::NONE ))
  {
    //If RIL restarts & when process is UP, Screen goes OFF.
    //We do not deregister as it is a NO OP at modem.
    Log::getInstance().d("[DSDModemEndPoint:: Already in Deregistered state."
                         " Not sending deregister request");
    return Message::Callback::Status::SUCCESS;
  }
  if( screenEnabled ) {
    Log::getInstance().d("[DSDModemEndPoint::register for UI indication");
    qmiReq.report_ui_changes_valid = true;
    qmiReq.report_ui_changes = 1;
    qmiReq.suppress_so_change_valid = true;
    qmiReq.suppress_so_change = 1;
    qmiReq.suppress_null_bearer_reason_valid = true;
    qmiReq.suppress_null_bearer_reason = 1;
  } else {
    Log::getInstance().d("[DSDModemEndPointModule::deregister for UI indication");
    qmiReq.report_ui_changes_valid = true;
    qmiReq.report_ui_changes = 0;
  }
  Message::Callback::Status status = sendRegisterForUiChangeInd(qmiReq);
  if (status == Message::Callback::Status::SUCCESS) {
    mScreenState = ( screenEnabled ) ? ScreenState_t::SCREEN_ON : ScreenState_t::SCREEN_OFF;
  }
  if (mScreenState == ScreenState_t::SCREEN_ON) {
    updateInitialUiInfo();
  }
  return status;
}

void DSDModemEndPoint::registerForUiChangeInd()
{
  if( mScreenState == ScreenState_t::SCREEN_ON )
  {
    dsd_register_ui_info_change_req_msg_v01 qmiReq;
    memset(&qmiReq, 0, sizeof(qmiReq));

    qmiReq.suppress_so_change_valid = true;
    qmiReq.suppress_so_change = 1;
    qmiReq.suppress_null_bearer_reason_valid = true;
    qmiReq.suppress_null_bearer_reason = 1;

    Log::getInstance().d("[DSDModemEndPoint:: Screen State is ON , register for UI indication");
    qmiReq.report_ui_changes_valid = true;
    qmiReq.report_ui_changes = 1;

    sendRegisterForUiChangeInd(qmiReq);
    updateInitialUiInfo();
  }
}

Message::Callback::Status DSDModemEndPoint::sendRegisterForUiChangeInd(dsd_register_ui_info_change_req_msg_v01 qmiReq)
{
  Message::Callback::Status status = Message::Callback::Status::FAILURE;
  dsd_register_ui_info_change_resp_msg_v01 qmiResp;
  memset(&qmiResp, 0, sizeof(qmiResp));

  auto retVal = sendRawSync(QMI_DSD_REGISTER_UI_INFO_CHANGE_REQ_V01,
                            (void *)&qmiReq, sizeof(qmiReq),
                            (void *)&qmiResp, sizeof(qmiResp),
                            QCRIL_DATA_QMI_TIMEOUT);

  if (retVal == QMI_NO_ERR) {
    status = Message::Callback::Status::SUCCESS;
  }
  return status;
}

void DSDModemEndPoint::updateInitialUiInfo()
{
  auto msg = std::make_shared<GetDataNrIconTypeMessage>();
  GenericCallback<NrIconType_t> cb([](std::shared_ptr<Message>,
                                      Message::Callback::Status status,
                                      std::shared_ptr<NrIconType_t> rsp) -> void {
    if (rsp != nullptr) {
      Log::getInstance().d("updateInitialUiInfo cb invoked status " + std::to_string((int)status));
      auto indMsg = std::make_shared<DataNrIconTypeIndMessage>(*rsp);
      indMsg->broadcast();
    }
  });
  msg->setCallback(&cb);
  msg->dispatch();
}

#ifdef RIL_FOR_LOW_RAM
RIL_Errno DSDModemEndPoint::setIsDataEnabled(bool enabled)
{
  RIL_Errno errNum =  RIL_E_GENERIC_FAILURE;
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    dsd_notify_data_settings_req_msg_v01  qmiReq;
    dsd_notify_data_settings_resp_msg_v01 qmiResp;

    memset(&qmiReq, 0, sizeof(qmiReq));
    memset(&qmiResp, 0, sizeof(qmiResp));
    qmiReq.data_service_switch_valid = TRUE;
    qmiReq.data_service_switch = enabled;

    Log::getInstance().d("[DSDModemEndPoint::Setting data_enabled =" + std::to_string(enabled));

    auto retVal = sendRawSync(QMI_DSD_NOTIFY_DATA_SETTING_REQ_V01,
                              (void *)&qmiReq, sizeof(qmiReq),
                              (void *)&qmiResp, sizeof(qmiResp),
                              QCRIL_DATA_QMI_TIMEOUT);
    if (retVal != QMI_NO_ERR) {
      Log::getInstance().d("[DSDModemEndPoint] Failed to send QMI_DSD_NOTIFY_DATA_SETTING_REQ_V01");
    } else {
        errNum = RIL_E_SUCCESS;
    }
  } else {
    Log::getInstance().d("[DSDModemEndPoint::setIsDataEnabled] Failed to send QMI message");
  }
  return errNum;
}

RIL_Errno DSDModemEndPoint::setIsDataRoamingEnabled(bool enabled)
{
  RIL_Errno errNum =  RIL_E_GENERIC_FAILURE;
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    dsd_notify_data_settings_req_msg_v01  qmiReq;
    dsd_notify_data_settings_resp_msg_v01 qmiResp;

    memset(&qmiReq, 0, sizeof(qmiReq));
    memset(&qmiResp, 0, sizeof(qmiResp));
    qmiReq.data_service_roaming_switch_valid = TRUE;
    qmiReq.data_service_roaming_switch = enabled;

    Log::getInstance().d("Setting Data Roaming Enabled =" + std::to_string(enabled));

    auto retVal = sendRawSync(QMI_DSD_NOTIFY_DATA_SETTING_REQ_V01,
                              (void *)&qmiReq, sizeof(qmiReq),
                              (void *)&qmiResp, sizeof(qmiResp),
                              QCRIL_DATA_QMI_TIMEOUT);
    if (retVal != QMI_NO_ERR) {
      Log::getInstance().d("[DSDModemEndPoint] Failed to send QMI_DSD_NOTIFY_DATA_SETTING_REQ_V01");
    } else {
        errNum = RIL_E_SUCCESS;
    }
  } else {
    Log::getInstance().d("SetIsDataRoamingEnabled Failed to send QMI message");
  }
  return errNum;
}

bool DSDModemEndPoint::deviceShutdownRequest()
{
  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    dsd_notify_data_settings_req_msg_v01 qmiReq;
    dsd_notify_data_settings_resp_msg_v01 qmiResp;

    memset(&qmiReq, 0, sizeof(dsd_notify_data_settings_req_msg_v01));
    memset(&qmiResp, 0, sizeof(dsd_notify_data_settings_resp_msg_v01));

    qmiReq.wifi_switch_valid = true;
    qmiReq.wifi_switch = false;

    Log::getInstance().d("QMI DSD Sending Device Shutdown Request ");

    auto retVal = sendRawSync(QMI_DSD_NOTIFY_DATA_SETTING_REQ_V01,
                                (void *)&qmiReq, sizeof(qmiReq),
                                (void *)&qmiResp, sizeof(qmiResp),
                                QCRIL_DATA_QMI_TIMEOUT);
    if (QMI_NO_ERR != retVal) {
      Log::getInstance().d("Sending of notify data setting failed retVal" + std::to_string(retVal));
      return false;
    }
    return true;
  } else {
    Log::getInstance().d("DSD EP not OPERATIONAL");
  }
  return false;
}

qmi_response_type_v01 DSDModemEndPoint::setQualityMeasurement
(
  dsd_set_quality_measurement_info_req_msg_v01 qmiReq
)
{
  qmi_response_type_v01 resp;
  memset(&resp, 0, sizeof(resp));
  resp.result = QMI_RESULT_FAILURE_V01;
  resp.error = QMI_ERR_INTERNAL_V01;

  if (getState() == ModemEndPoint::State::OPERATIONAL)
  {
    dsd_set_quality_measurement_info_resp_msg_v01 qmiResp;
    memset(&qmiResp, 0, sizeof(qmiResp));

    auto rc = sendRawSync(QMI_DSD_SET_QUALITY_MEASUREMENT_INFO_REQ_V01,
                                  (void *)&qmiReq, sizeof(qmiReq),
                                  (void *)&qmiResp, sizeof(qmiResp),
                                  QCRIL_DATA_QMI_TIMEOUT);

    if (QMI_NO_ERR != rc) {
      Log::getInstance().d("failed to send QMI msg rc= "+std::to_string(rc));
    } else {
      resp = qmiResp.resp;
    }
  } else {
    Log::getInstance().d("DSD EP not OPERATIONAL");
  }
  Log::getInstance().d("qcril_data_set_quality_measurement():"
          " result"+std::to_string(resp.result)+" err=" +std::to_string(resp.error));
  return resp;
}
#endif//RIL_FOR_LOW_RAM
