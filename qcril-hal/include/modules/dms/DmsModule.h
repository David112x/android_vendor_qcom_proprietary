/******************************************************************************
#  Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/

#pragma once
#include "framework/Module.h"
#include <framework/AddPendingMessageList.h>
#include "modules/qmi/DmsModemEndPoint.h"
#include "modules/qmi/QmiAsyncResponseMessage.h"
#include "modules/qmi/EndpointStatusIndMessage.h"

#include "modules/dms/IpcRadioPowerStateMesage.h"

#include <modules/android/RilRequestMessage.h>

#include "interfaces/dms/RilRequestRadioPowerMessage.h"
#include "interfaces/dms/RilRequestGetModemActivityMessage.h"
#include "interfaces/dms/RilRequestGetDeviceIdentityMessage.h"
#include "interfaces/dms/RilRequestGetBaseBandVersionMessage.h"

#include "framework/GenericCallback.h"
#include "framework/SolicitedMessage.h"
#include "framework/Message.h"
#include "framework/add_message_id.h"

class DmsModule : public Module, public AddPendingMessageList {
  public:
    DmsModule();
    ~DmsModule();
    void init();
#ifdef QMI_RIL_UTF
    void qcrilHalDmsModuleCleanup();
#endif
    void broadcastIpcRadioPowerStateMesage(int is_genuine_signal);

  private:
    bool mReady = false;
    bool mIsIpcReady = false;
    std::shared_ptr<IpcRadioPowerStateMesage> mIpcRadioPowerStateMsg = nullptr;
    qtimutex::QtiRecursiveMutex mMutex;

  private:

    void handleQcrilInit(std::shared_ptr<Message> msg);
    void handleDmsEndpointStatusIndMessage(std::shared_ptr<Message> msg);

    void handleIpcRadioPowerStateMesage(std::shared_ptr<IpcRadioPowerStateMesage> shared_msg);

    void handleRadioPowerRequest(std::shared_ptr<RilRequestRadioPowerMessage> shared_msg);
    void handleGetActivityInfoRequest(std::shared_ptr<RilRequestGetModemActivityMessage> shared_msg);
    void handleBaseBandVersionRequest(std::shared_ptr<RilRequestGetBaseBandVersionMessage> shared_msg);
    void handleDeviceIdentiyRequest(std::shared_ptr<RilRequestGetDeviceIdentityMessage> shared_msg);
};

DmsModule &getDmsModule();

