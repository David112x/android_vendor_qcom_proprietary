/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#include "modules/qmi/LteModemEndPoint.h"

#include "common_v01.h"
#include "device_management_service_v01.h"
#include "lte_v01.h"
#include "modules/qmi/GetOperatingModeSyncMessage.h"
#include "modules/qmi/QmiServiceUpIndMessage.h"
#include "modules/qmi/QmiSetupRequest.h"

using std::to_string;
constexpr const char *LteModemEndPoint::NAME;

void LteModemEndPoint::requestSetup(string clientToken, GenericCallback<string> *callback) {
  auto shared_setupMsg = std::make_shared<QmiSetupRequest>(clientToken, 0, nullptr, callback);
  mModule->dispatch(shared_setupMsg);
}
