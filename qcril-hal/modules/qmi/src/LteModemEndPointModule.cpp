/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#include "modules/qmi/LteModemEndPointModule.h"
#include "framework/ThreadPoolAgent.h"
#include "modules/qmi/GetOperatingModeSyncMessage.h"
#include "modules/qmi/LteModemEndPoint.h"
#include "modules/qmi/QmiSendRawAsyncMessage.h"
#include "modules/qmi/QmiSendRawSyncMessage.h"
#include "modules/qmi/QmiServiceUpIndMessage.h"
#include "qmi_client.h"
#include <cstring>

LteModemEndPointModule::LteModemEndPointModule(string name, ModemEndPoint &owner)
    : ModemEndPointModule(name, owner) {
  mServiceObject = nullptr;
#ifdef RIL_FOR_LOW_RAM
  mLooper = std::unique_ptr<ThreadPoolAgent>(new ThreadPoolAgent);
#else
  mLooper = std::unique_ptr<ModuleLooper>(new ModuleLooper);
#endif
  using std::placeholders::_1;
  mMessageHandler = {};
}

LteModemEndPointModule::~LteModemEndPointModule() { mLooper = nullptr; }

qmi_idl_service_object_type LteModemEndPointModule::getServiceObject() {
  return lte_get_service_object_v01();
}

bool LteModemEndPointModule::handleQmiBinding(qcril_instance_id_e_type instanceId, int8_t stackId) {
  if (stackId < 0) {
    return false;
  }

  qmi_lte_bind_subscription_req_msg_v01 lte_bind_request = {};
  qmi_lte_bind_subscription_resp_msg_v01 lte_bind_resp = {};

  if (stackId == 0) {
    lte_bind_request.subscription = LTE_PRIMARY_SUBSCRIPTION_V01;
  } else if (stackId == 1) {
    lte_bind_request.subscription = LTE_SECONDARY_SUBSCRIPTION_V01;
  } else if (stackId == 2) {
    lte_bind_request.subscription = LTE_TERTIARY_SUBSCRIPTION_V01;
  } else {
    return false;
  }

  int ntries = 0;
  do {
    qmi_client_error_type res = qmi_client_send_msg_sync(
        mQmiSvcClient, QMI_LTE_BIND_SUBSCRIPTION_REQ_V01, (void *)&lte_bind_request,
        sizeof(lte_bind_request), (void *)&lte_bind_resp, sizeof(lte_bind_resp),
        ModemEndPoint::DEFAULT_SYNC_TIMEOUT);
    if (QMI_NO_ERR == res && lte_bind_resp.resp.result == QMI_RESULT_SUCCESS_V01) {
      Log::getInstance().d("[LteModemEndPointModule]: QMI binding succeeds. instanceId: " +
                           std::to_string((int)instanceId) +
                           " stackId: " + std::to_string(stackId));
      return true;
    }
    else if(QMI_SERVICE_ERR == res)
    {
        return false;
    }
    usleep(500 * 1000);
  } while (++ntries < 10);

  return false;
}
