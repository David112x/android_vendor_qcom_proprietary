/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include "common_v01.h"
#include "device_management_service_v01.h"
#include "lte_v01.h"
#include "qmi_client.h"

#include "modules/qmi/ModemEndPoint.h"
#include "modules/qmi/ModemEndPointModule.h"
#include "modules/qmi/QmiServiceUpIndMessage.h"
#include "modules/qmi/QmiSetupRequest.h"

class LteModemEndPointModule : public ModemEndPointModule {
public:
  LteModemEndPointModule(string name, ModemEndPoint &owner);
  ~LteModemEndPointModule();

private:
  qmi_idl_service_object_type getServiceObject() override;
  bool handleQmiBinding(qcril_instance_id_e_type instanceId, int8_t stackId) override;
};
