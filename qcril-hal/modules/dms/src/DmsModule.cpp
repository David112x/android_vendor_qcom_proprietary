/******************************************************************************
#  Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#include <cstring>
#include <framework/ModuleLooper.h>
#include <framework/QcrilInitMessage.h>
#include <framework/PolicyManager.h>
#include <framework/SingleDispatchRestriction.h>
#include <framework/SingleDispatchRestrictionHandleDuplicate.h>
#include <framework/ThreadPoolManager.h>
#include <qtibus/Messenger.h>

#include <modules/android/ril_message_factory.h>

#include "qcril_log.h"

#include "modules/nas/qcril_qmi_nas.h"
#include "modules/dms/DmsModule.h"
#include "telephony/ril.h"

#undef TAG
#define TAG "RILQ"

static load_module<DmsModule> dms_module;

DmsModule &getDmsModule() {
    return (dms_module.get_module());
}

/*
 * 1. Indicate your preference for looper.
 * 2. Subscribe to the list of messages via mMessageHandler.
 * 3. Follow RAII practice.
 */
DmsModule::DmsModule() : AddPendingMessageList("DmsModule") {
  mName = "DmsModule";

  using std::placeholders::_1;
  mMessageHandler = {
    HANDLER(QcrilInitMessage, DmsModule::handleQcrilInit),
    HANDLER(IpcRadioPowerStateMesage, DmsModule::handleIpcRadioPowerStateMesage),

    HANDLER(RilRequestRadioPowerMessage, DmsModule::handleRadioPowerRequest),
    HANDLER(RilRequestGetModemActivityMessage, DmsModule::handleGetActivityInfoRequest),
    HANDLER(RilRequestGetBaseBandVersionMessage, DmsModule::handleBaseBandVersionRequest),
    HANDLER(RilRequestGetDeviceIdentityMessage, DmsModule::handleDeviceIdentiyRequest),

    // End Point Status Indication
    {REG_MSG("DMS_ENDPOINT_STATUS_IND"),
             std::bind(&DmsModule::handleDmsEndpointStatusIndMessage, this, _1)},
  };
}

/* Follow RAII.
*/
DmsModule::~DmsModule() {}

/*
 * Module specific initialization that does not belong to RAII .
 */
void DmsModule::init() {
    Module::init();

    PolicyManager::getInstance().setMessageRestriction(std::vector<std::string>{
        RilRequestGetModemActivityMessage::MESSAGE_NAME},
        std::make_shared<SingleDispatchRestrictionHandleDuplicate>());

    PolicyManager::getInstance().setMessageRestriction(std::vector<std::string>{
        RilRequestGetBaseBandVersionMessage::MESSAGE_NAME},
        std::make_shared<SingleDispatchRestrictionHandleDuplicate>());

    PolicyManager::getInstance().setMessageRestriction(std::vector<std::string>{
        RilRequestGetDeviceIdentityMessage::MESSAGE_NAME},
        std::make_shared<SingleDispatchRestrictionHandleDuplicate>());

    PolicyManager::getInstance().setMessageRestriction(std::vector<std::string>{
        RilRequestRadioPowerMessage::MESSAGE_NAME},
        std::make_shared<SingleDispatchRestriction>());

    Messenger &msgr = Messenger::get();
    msgr.setOnClientConnectedCb([](pid_t pid) {
      QCRIL_LOG_DEBUG("Client connected. pid = %d", pid);
      Messenger::get().registerForMessage(
          IpcRadioPowerStateMesage::get_class_message_id(),
          [](IPCIStream &is) -> std::shared_ptr<IPCMessage> {
            auto msg = std::make_shared<IpcRadioPowerStateMesage>();
            if (msg) {
              msg->deserialize(is);
            }
            return msg;
          });
    });
    msgr.setOnClientDeadCb([this](pid_t pid) {
      QCRIL_LOG_DEBUG("Client disconnected. pid = %d", pid);
      mIsIpcReady = false;
    });
    msgr.setOnRemoteRegistrationCb([this](pid_t sender, message_id_t msgId) {
      QCRIL_LOG_DEBUG("Remote client %d registered for message %s", sender,
                      (msgId ? msgId->get_name().c_str() : ""));
      if (msgId->get_name() == IpcRadioPowerStateMesage::MESSAGE_NAME) {
        mIsIpcReady = true;
        // Broadcast if there is any pending message
        if (mIpcRadioPowerStateMsg) {
          mIpcRadioPowerStateMsg->broadcast();
          mIpcRadioPowerStateMsg = nullptr;
        }
      }
    });
}

void DmsModule::handleQcrilInit(std::shared_ptr<Message> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
}

void DmsModule::handleIpcRadioPowerStateMesage(
    std::shared_ptr<IpcRadioPowerStateMesage> msg) {
  QCRIL_LOG_FUNC_ENTRY("msg = %s", (msg ? msg->dump().c_str() : "nullptr"));
  // Handle the messages from the remote instance
  if (msg->getIsRemote()) {
    qcril_qmi_nas_handle_multiple_rild_radio_power_state_propagation(
        msg->getIsGenuineSignal());
  }
}

void DmsModule::handleDmsEndpointStatusIndMessage(std::shared_ptr<Message> msg) {
    auto shared_indMsg(std::static_pointer_cast<EndpointStatusIndMessage>(msg));

    Log::getInstance().d("[" + mName + "]: Handling msg = " + msg->dump());

    if (shared_indMsg->getState() == ModemEndPoint::State::OPERATIONAL) {
        mReady = true;
        qcril_qmi_nas_notify_radio_power_if_dms_up();
        qcril_qmi_nas_trigger_bootup_optimization();
    } else {
        // state is not operational
        mReady = false;
        clearPendingMessageList();
    }
}

void qcril_qmi_nas_request_power_wrapper(void *cb_data)
{
    auto schedCbData = static_cast<ScheduleCallbackData*>(cb_data);
    auto msg(std::static_pointer_cast<RilRequestRadioPowerMessage>(schedCbData->getData()));
    delete schedCbData;
    qcril_qmi_nas_request_power(msg);
}


void DmsModule::handleRadioPowerRequest(std::shared_ptr<RilRequestRadioPowerMessage> shared_msg) {
    Log::getInstance().d("[" + mName + "]: Handling msg = " + shared_msg->dump());

    auto cbData = new ScheduleCallbackData(shared_msg);
    if (cbData) {
        ThreadPoolManager::getInstance().scheduleExecution(
            qcril_qmi_nas_request_power_wrapper, cbData);
    } else {
        shared_msg->sendResponse(shared_msg, Message::Callback::Status::SUCCESS,
            std::make_shared<QcRilRequestMessageCallbackPayload>(RIL_E_NO_MEMORY, nullptr));
    }
}


void DmsModule::handleGetActivityInfoRequest(std::shared_ptr<RilRequestGetModemActivityMessage> shared_msg) {
    Log::getInstance().d("[" + mName + "]: Handling msg = " + shared_msg->dump());
    if (mReady) {
        qcril_qmi_nas_dms_get_activity_info(shared_msg);
    } else {
        shared_msg->sendResponse(shared_msg, Message::Callback::Status::SUCCESS,
            std::make_shared<QcRilRequestMessageCallbackPayload>(RIL_E_RADIO_NOT_AVAILABLE , nullptr));
    }
}


void DmsModule::handleBaseBandVersionRequest(std::shared_ptr<RilRequestGetBaseBandVersionMessage> shared_msg) {
    Log::getInstance().d("[" + mName + "]: Handling msg = " + shared_msg->dump());
    if (mReady) {
        qcril_qmi_nas_dms_request_baseband_version(shared_msg);
    } else {
        shared_msg->sendResponse(shared_msg, Message::Callback::Status::SUCCESS,
            std::make_shared<QcRilRequestMessageCallbackPayload>(RIL_E_RADIO_NOT_AVAILABLE , nullptr));
    }
}


void DmsModule::handleDeviceIdentiyRequest(std::shared_ptr<RilRequestGetDeviceIdentityMessage> shared_msg) {
    Log::getInstance().d("[" + mName + "]: Handling msg = " + shared_msg->dump());
    if (mReady) {
        qcril_qmi_nas_dms_request_device_identity(shared_msg);
    } else {
        shared_msg->sendResponse(shared_msg, Message::Callback::Status::SUCCESS,
            std::make_shared<QcRilRequestMessageCallbackPayload>(RIL_E_RADIO_NOT_AVAILABLE , nullptr));
    }
}

void DmsModule::broadcastIpcRadioPowerStateMesage(int is_genuine_signal) {
  QCRIL_LOG_FUNC_ENTRY();
  auto msg = std::make_shared<IpcRadioPowerStateMesage>(is_genuine_signal);
  QCRIL_LOG_DEBUG("mIsIpcReady = %s", (mIsIpcReady ? "true" : "false"));
  if (mIsIpcReady) {
    if (msg) {
      msg->broadcast();
    }
  } else {
    QCRIL_LOG_DEBUG("IPC is not ready., queueing the msg");
    mIpcRadioPowerStateMsg = msg;
  }
  QCRIL_LOG_FUNC_RETURN();
}
