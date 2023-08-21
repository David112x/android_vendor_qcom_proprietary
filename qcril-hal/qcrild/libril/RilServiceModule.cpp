/******************************************************************************
#  Copyright (c) 2018,2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/

#define TAG "RilServiceModule"
#include "RilServiceModule.h"

namespace android {
extern void storeNITZTimeData(const std::string& time);
}

RilServiceModule::RilServiceModule(qcril_instance_id_e_type instance)
        : mInstance(instance) {
    mName = "RilServiceModule";

    using std::placeholders::_1;
    mMessageHandler = {
        HANDLER(QcrilInitMessage, RilServiceModule::handleQcrilInit),
        HANDLER(RilUnsolIncoming3GppSMSMessage, RilServiceModule::handleIncoming3GppSMSMessage),
        HANDLER(RilUnsolIncoming3Gpp2SMSMessage, RilServiceModule::handleIncoming3Gpp2SMSMessage),
        HANDLER(RilUnsolNewSmsOnSimMessage, RilServiceModule::handleNewSmsOnSimMessage),
        HANDLER(RilUnsolNewSmsStatusReportMessage, RilServiceModule::handleNewSmsStatusReportMessage),
        HANDLER(RilUnsolNewBroadcastSmsMessage, RilServiceModule::handleNewBroadcastSmsMessage),
        HANDLER(RilUnsolStkCCAlphaNotifyMessage, RilServiceModule::handleStkCCAlphaNotifyMessage),
        HANDLER(RilUnsolCdmaRuimSmsStorageFullMessage, RilServiceModule::handleCdmaRuimSmsStorageFullMessage),
        HANDLER(RilUnsolSimSmsStorageFullMessage, RilServiceModule::handleSimSmsStorageFullMessage),
        HANDLER(RilUnsolImsNetworkStateChangedMessage, RilServiceModule::handleImsNetworkStateChangedMessage),

        HANDLER(QcRilUnsolCallStateChangeMessage, RilServiceModule::handleQcRilUnsolCallStateChangeMessage),
        HANDLER(QcRilUnsolCallRingingMessage, RilServiceModule::handleQcRilUnsolCallRingingMessage),
        HANDLER(QcRilUnsolSupplementaryServiceMessage, RilServiceModule::handleQcRilUnsolSupplementaryServiceMessage),
        HANDLER(QcRilUnsolSrvccStatusMessage, RilServiceModule::handleQcRilUnsolSrvccStatusMessage),
        HANDLER(QcRilUnsolRingbackToneMessage, RilServiceModule::handleQcRilUnsolRingbackToneMessage),
        HANDLER(QcRilUnsolCdmaOtaProvisionStatusMessage, RilServiceModule::handleQcRilUnsolCdmaOtaProvisionStatusMessage),
        HANDLER(QcRilUnsolCdmaCallWaitingMessage, RilServiceModule::handleQcRilUnsolCdmaCallWaitingMessage),
        HANDLER(QcRilUnsolSuppSvcNotificationMessage, RilServiceModule::handleQcRilUnsolSuppSvcNotificationMessage),
        HANDLER(QcRilUnsolOnUssdMessage, RilServiceModule::handleQcRilUnsolOnUssdMessage),
        HANDLER(QcRilUnsolCdmaInfoRecordMessage, RilServiceModule::handleQcRilUnsolCdmaInfoRecordMessage),

        HANDLER(RilAcknowledgeRequestMessage, RilServiceModule::handleAcknowledgeRequestMessage),
        HANDLER(RilUnsolNetworkStateChangedMessage, RilServiceModule::handleNetworkStateChangedMessage),
        HANDLER(RilUnsolNitzTimeReceivedMessage, RilServiceModule::handleNitzTimeReceivedMessage),
        HANDLER(RilUnsolVoiceRadioTechChangedMessage, RilServiceModule::handleVoiceRadioTechChangedMessage),
        HANDLER(RilUnsolNetworkScanResultMessage, RilServiceModule::handleNetworkScanResultMessage),
        HANDLER(RilUnsolSignalStrengthMessage, RilServiceModule::handleSignalStrengthMessage),
        HANDLER(RilUnsolEmergencyCallbackModeMessage, RilServiceModule::handleEmergencyCallbackModeMessage),
        HANDLER(RilUnsolRadioCapabilityMessage, RilServiceModule::handlelRadioCapabilityMessage),
        HANDLER(RilUnsolCdmaPrlChangedMessage, RilServiceModule::handleCdmaPrlChangedMessage),
        HANDLER(RilUnsolRestrictedStateChangedMessage, RilServiceModule::handleRestrictedStateChangedMessage),
        HANDLER(RilUnsolUiccSubsStatusChangedMessage, RilServiceModule::handleUiccSubsStatusChangedMessage),
        HANDLER(RilUnsolRadioStateChangedMessage, RilServiceModule::handleRadioStateChangedMessage),

        HANDLER(UimSimStatusChangedInd, RilServiceModule::handleUimSimStatusChangedInd),
        HANDLER(UimSimRefreshIndication, RilServiceModule::handleUimSimRefreshIndication),
        HANDLER(GstkUnsolIndMsg, RilServiceModule::handleGstkUnsolIndMsg),

#ifdef RIL_FOR_LOW_RAM
        HANDLER(rildata::SetupDataCallRadioResponseIndMessage_1_0, RilServiceModule::handleSetupDataCallRadioResponseIndMessage),
        HANDLER(rildata::DeactivateDataCallRadioResponseIndMessage_1_0, RilServiceModule::handleDeactivateDataCallRadioResponseIndMessage),
        HANDLER(rildata::RadioKeepAliveStatusIndMessage,RilServiceModule::handleRadioKeepAliveStatusIndMessage),
        HANDLER(rildata::RadioDataCallListChangeIndMessage, RilServiceModule::handleRadioDataCallListChangeIndMessage),
#endif
    };
}

sp<RadioImpl> RilServiceModule::getRadioImpl()
{
    auto rs = getRadioService(mInstance);
    return rs;
}

void RilServiceModule::handleQcrilInit(std::shared_ptr<QcrilInitMessage> msg) {
    QCRIL_LOG_INFO("Handling QcrilInitMessage %s", msg->dump().c_str());
}

void RilServiceModule::handleIncoming3GppSMSMessage(
        std::shared_ptr<RilUnsolIncoming3GppSMSMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendNewSms(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleIncoming3Gpp2SMSMessage(
        std::shared_ptr<RilUnsolIncoming3Gpp2SMSMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendNewCdmaSms(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleNewSmsOnSimMessage(
        std::shared_ptr<RilUnsolNewSmsOnSimMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendNewSmsOnSim(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleNewSmsStatusReportMessage(
        std::shared_ptr<RilUnsolNewSmsStatusReportMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendNewSmsStatusReport(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleNewBroadcastSmsMessage(
        std::shared_ptr<RilUnsolNewBroadcastSmsMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendNewBroadcastSms(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleStkCCAlphaNotifyMessage(
        std::shared_ptr<RilUnsolStkCCAlphaNotifyMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendStkCCAlphaNotify(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleCdmaRuimSmsStorageFullMessage(
        std::shared_ptr<RilUnsolCdmaRuimSmsStorageFullMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendCdmaRuimSmsStorageFull(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleSimSmsStorageFullMessage(
        std::shared_ptr<RilUnsolSimSmsStorageFullMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendSimSmsStorageFull(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleImsNetworkStateChangedMessage(
        std::shared_ptr<RilUnsolImsNetworkStateChangedMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendImsNetworkStateChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleQcRilUnsolCallRingingMessage(
    std::shared_ptr<QcRilUnsolCallRingingMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  if (msg && !msg->isIms()) {
    auto ri = getRadioImpl();
    if (ri) {
      // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
      android::grabPartialWakeLock();

      auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
      radioServiceRwlockPtr->lock_shared();

      ri->sendCallRing(msg);

      radioServiceRwlockPtr->unlock_shared();
    }
  }
}

void RilServiceModule::handleQcRilUnsolCallStateChangeMessage(
    std::shared_ptr<QcRilUnsolCallStateChangeMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  if (msg && !msg->isIms()) {
    auto ri = getRadioImpl();
    if (ri) {
      // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
      android::grabPartialWakeLock();

      auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
      radioServiceRwlockPtr->lock_shared();

      ri->sendCallStateChanged(msg);

      radioServiceRwlockPtr->unlock_shared();
    }
  }
}

void RilServiceModule::handleQcRilUnsolSupplementaryServiceMessage(
    std::shared_ptr<QcRilUnsolSupplementaryServiceMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  if (msg && !msg->isIms()) {
    auto ri = getRadioImpl();
    if (ri) {
      // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
      android::grabPartialWakeLock();

      auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
      radioServiceRwlockPtr->lock_shared();

      ri->sendOnSupplementaryServiceIndication(msg);

      radioServiceRwlockPtr->unlock_shared();
    }
  }
}
void RilServiceModule::handleQcRilUnsolSrvccStatusMessage(
    std::shared_ptr<QcRilUnsolSrvccStatusMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  auto ri = getRadioImpl();
  if (ri) {
    // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
    android::grabPartialWakeLock();

    auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
    radioServiceRwlockPtr->lock_shared();

    ri->sendSrvccStateNotify(msg);

    radioServiceRwlockPtr->unlock_shared();
  }
}
void RilServiceModule::handleQcRilUnsolRingbackToneMessage(
    std::shared_ptr<QcRilUnsolRingbackToneMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  if (msg && !msg->isIms()) {
    auto ri = getRadioImpl();
    if (ri) {
      // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
      android::grabPartialWakeLock();

      auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
      radioServiceRwlockPtr->lock_shared();

      ri->sendIndicateRingbackTone(msg);

      radioServiceRwlockPtr->unlock_shared();
    }
  }
}
void RilServiceModule::handleQcRilUnsolCdmaOtaProvisionStatusMessage(
    std::shared_ptr<QcRilUnsolCdmaOtaProvisionStatusMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  auto ri = getRadioImpl();
  if (ri) {
    // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
    android::grabPartialWakeLock();

    auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
    radioServiceRwlockPtr->lock_shared();

    ri->sendCdmaOtaProvisionStatus(msg);

    radioServiceRwlockPtr->unlock_shared();
  }
}
void RilServiceModule::handleQcRilUnsolCdmaCallWaitingMessage(
    std::shared_ptr<QcRilUnsolCdmaCallWaitingMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  auto ri = getRadioImpl();
  if (ri) {
    // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
    android::grabPartialWakeLock();

    auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
    radioServiceRwlockPtr->lock_shared();

    ri->sendCdmaCallWaiting(msg);

    radioServiceRwlockPtr->unlock_shared();
  }
}
void RilServiceModule::handleQcRilUnsolSuppSvcNotificationMessage(
    std::shared_ptr<QcRilUnsolSuppSvcNotificationMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  if (msg && !msg->isIms()) {
    auto ri = getRadioImpl();
    if (ri) {
      // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
      android::grabPartialWakeLock();

      auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
      radioServiceRwlockPtr->lock_shared();

      ri->sendSuppSvcNotify(msg);

      radioServiceRwlockPtr->unlock_shared();
    }
  }
}
void RilServiceModule::handleQcRilUnsolOnUssdMessage(std::shared_ptr<QcRilUnsolOnUssdMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  auto ri = getRadioImpl();
  if (ri) {
    // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
    android::grabPartialWakeLock();

    auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
    radioServiceRwlockPtr->lock_shared();

    ri->sendOnUssd(msg);

    radioServiceRwlockPtr->unlock_shared();
  }
}
void RilServiceModule::handleQcRilUnsolCdmaInfoRecordMessage(std::shared_ptr<QcRilUnsolCdmaInfoRecordMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

  auto ri = getRadioImpl();
  if (ri) {
    // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
    android::grabPartialWakeLock();

    auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
    radioServiceRwlockPtr->lock_shared();

    ri->sendCdmaInfoRec(msg);

    radioServiceRwlockPtr->unlock_shared();
  }
}

void RilServiceModule::handleUimSimStatusChangedInd(
        std::shared_ptr<UimSimStatusChangedInd> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendSimStatusChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleUimSimRefreshIndication(
        std::shared_ptr<UimSimRefreshIndication> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendSimRefresh(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleGstkUnsolIndMsg(
        std::shared_ptr<GstkUnsolIndMsg> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendGstkIndication(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleAcknowledgeRequestMessage(
        std::shared_ptr<RilAcknowledgeRequestMessage> msg) {
    auto ri = getRadioImpl();
    if (ri) {
        ri->sendAcknowledgeRequest(msg);
    }
}

void RilServiceModule::handleNetworkStateChangedMessage(
        std::shared_ptr<RilUnsolNetworkStateChangedMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendNetworkStateChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleNitzTimeReceivedMessage(
        std::shared_ptr<RilUnsolNitzTimeReceivedMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    int retVal = 0;
    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        retVal = ri->sendNitzTimeReceived(msg);

        radioServiceRwlockPtr->unlock_shared();
    }

    if (retVal != 0) {
        QCRIL_LOG_INFO("store the last reported NITZ time");
        android::storeNITZTimeData(msg->getNitzTime());
    }
}

void RilServiceModule::handleVoiceRadioTechChangedMessage(
        std::shared_ptr<RilUnsolVoiceRadioTechChangedMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendVoiceRadioTechChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleNetworkScanResultMessage(
        std::shared_ptr<RilUnsolNetworkScanResultMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendNetworkScanResult(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleSignalStrengthMessage(
        std::shared_ptr<RilUnsolSignalStrengthMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendSignalStrength(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleEmergencyCallbackModeMessage(
        std::shared_ptr<RilUnsolEmergencyCallbackModeMessage> msg) {
    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendEmergencyCallbackMode(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handlelRadioCapabilityMessage(
        std::shared_ptr<RilUnsolRadioCapabilityMessage> msg) {
    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendRadioCapability(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleCdmaPrlChangedMessage(
        std::shared_ptr<RilUnsolCdmaPrlChangedMessage> msg) {
    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendCdmaPrlChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleRestrictedStateChangedMessage(
        std::shared_ptr<RilUnsolRestrictedStateChangedMessage> msg) {
    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendRestrictedStateChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleUiccSubsStatusChangedMessage(
        std::shared_ptr<RilUnsolUiccSubsStatusChangedMessage> msg) {
    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendUiccSubsStatusChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

void RilServiceModule::handleRadioStateChangedMessage(
        std::shared_ptr<RilUnsolRadioStateChangedMessage> msg) {
    auto ri = getRadioImpl();
    if (ri) {
        // The ATEL will acknowldge this UNSOL, which leads to release this wakelock
        android::grabPartialWakeLock();

        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        ri->sendRadioStateChanged(msg);

        radioServiceRwlockPtr->unlock_shared();
    }
}

#ifdef RIL_FOR_LOW_RAM
void RilServiceModule::handleRadioDataCallListChangeIndMessage(
    std::shared_ptr<rildata::RadioDataCallListChangeIndMessage> msg) {
    auto ri = getRadioImpl();
    if(msg && ri) {
        QCRIL_LOG_DEBUG("Handling handleRadioDataCallListChangeIndMessage %s", msg->dump().c_str());
        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();

        std::vector<rildata::DataCallResult_t> dcList = msg->getDCList();
        ::android::hardware::hidl_vec<V1_0::SetupDataCallResult> dcResultList;
        QCRIL_LOG_DEBUG("dcList %d",dcList.size());
        dcResultList.resize(dcList.size());
        int i=0;
        for (rildata::DataCallResult_t entry : dcList)
        {
            dcResultList[i].status = (V1_0::DataCallFailCause)entry.cause;
            dcResultList[i].suggestedRetryTime = entry.suggestedRetryTime;
            dcResultList[i].cid = entry.cid;
            dcResultList[i].active = entry.active;
            dcResultList[i].type = entry.type;
            dcResultList[i].ifname = entry.ifname;
            dcResultList[i].addresses = entry.addresses;
            dcResultList[i].dnses = entry.dnses;
            dcResultList[i].gateways = entry.gateways;
            dcResultList[i].pcscf = entry.pcscf;
            dcResultList[i].mtu = entry.mtu;
            i++;
        }
        static_cast<RadioImpl*>(ri.get())->dataCallListChanged(RadioIndicationType::UNSOLICITED, dcResultList);
        radioServiceRwlockPtr->unlock_shared();
    }
    else {
        QCRIL_LOG_DEBUG("Handling handleRadioDataCallListChangeIndMessage_1_0 is null");
    }
}

void RilServiceModule::handleRadioKeepAliveStatusIndMessage(std::shared_ptr<rildata::RadioKeepAliveStatusIndMessage> msg) {
    if(msg) {
      QCRIL_LOG_DEBUG("Handling RadioKeepAliveStatusIndMessage %s", msg->dump().c_str());
      auto ri = getRadioImpl();
      if (ri) {
        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        if(radioServiceRwlockPtr)
        {
          radioServiceRwlockPtr->lock_shared();
          V1_1::KeepaliveStatus status;
          status.sessionHandle = msg->getHandle();
          status.code = (::android::hardware::radio::V1_1::KeepaliveStatusCode)(msg->getStatusCode());

          static_cast<RadioImpl*>(ri.get())->KeepAliveStatusInd(RadioIndicationType::UNSOLICITED, status);
          radioServiceRwlockPtr->unlock_shared();
        } else {
          QCRIL_LOG_DEBUG("radioServiceRwlockPtr is NULL");
        }
      }
    } else {
        QCRIL_LOG_DEBUG("Handling RadioKeepAliveStatusIndicationMessage is null");
    }
}

void RilServiceModule::handleSetupDataCallRadioResponseIndMessage(
    std::shared_ptr<rildata::SetupDataCallRadioResponseIndMessage_1_0> msg) {
    auto ri = getRadioImpl();
    if(msg && ri) {
        QCRIL_LOG_DEBUG("Handling handleSetupDataCallRadioResponseIndMessage %s",msg->dump().c_str());
        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();
        auto rsp = msg->getResponse();
        auto status = msg->getStatus();
        auto serial = msg->getSerial();

        RadioResponseInfo responseInfo{.serial = serial, .error = RadioError::NO_MEMORY};
        V1_0::SetupDataCallResult dcResult = {};
        dcResult.status = V1_0::DataCallFailCause::ERROR_UNSPECIFIED;
        dcResult.suggestedRetryTime = -1;
        RadioError e = RadioError::NONE;
        if (status == Message::Callback::Status::SUCCESS)
        {
            QCRIL_LOG_DEBUG("setup data call cb invoked status %d respErr %d", status, rsp.respErr);
            QCRIL_LOG_DEBUG("cause = %d", rsp.call.cause);
            QCRIL_LOG_DEBUG("suggestedRetryTime = %d", rsp.call.suggestedRetryTime);
            QCRIL_LOG_DEBUG("cid = %d", rsp.call.cid);
            QCRIL_LOG_DEBUG("active = %d", rsp.call.active);
            QCRIL_LOG_DEBUG("type = %s", rsp.call.type.c_str());
            QCRIL_LOG_DEBUG("ifname = %s", rsp.call.ifname.c_str());
            QCRIL_LOG_DEBUG("addresses = %s", rsp.call.addresses.c_str());
            QCRIL_LOG_DEBUG("dnses = %s", rsp.call.dnses.c_str());
            QCRIL_LOG_DEBUG("gateways = %s", rsp.call.gateways.c_str());
            QCRIL_LOG_DEBUG("pcscf = %s", rsp.call.pcscf.c_str());
            QCRIL_LOG_DEBUG("mtu = %d", rsp.call.mtu);

            dcResult.status = convertDcFailStatusToHidlDcFailCause(rsp.call.cause);
            dcResult.suggestedRetryTime = rsp.call.suggestedRetryTime;
            dcResult.cid = rsp.call.cid;
            dcResult.active = rsp.call.active;
            dcResult.type = rsp.call.type;
            dcResult.ifname = rsp.call.ifname;
            dcResult.addresses = rsp.call.addresses;
            dcResult.dnses = rsp.call.dnses;
            dcResult.gateways = rsp.call.gateways;
            dcResult.pcscf = rsp.call.pcscf;
            dcResult.mtu = rsp.call.mtu;
        }
        else
        {
            switch (rsp.respErr)
            {
            case rildata::ResponseError_t::NOT_SUPPORTED:
                e = RadioError::REQUEST_NOT_SUPPORTED;
                break;
            case rildata::ResponseError_t::INVALID_ARGUMENT:
                e = RadioError::INVALID_ARGUMENTS;
                break;
            default:
                e = RadioError::GENERIC_FAILURE;
                break;
            }
        }
        responseInfo = {.serial = serial, .error = e};
        static_cast<RadioImpl*>(ri.get())->setupDataCallResponse(responseInfo, dcResult);
        radioServiceRwlockPtr->unlock_shared();
    }
}


void RilServiceModule::handleDeactivateDataCallRadioResponseIndMessage(
    std::shared_ptr<rildata::DeactivateDataCallRadioResponseIndMessage_1_0> msg) {
    auto ri = getRadioImpl();
    if(msg && ri) {
        QCRIL_LOG_DEBUG("Handling handleDeactivateDataCallRadioResponseIndMessage %s",msg->dump().c_str());
        auto radioServiceRwlockPtr = radio::getRadioServiceRwlock(ri->mSlotId);
        radioServiceRwlockPtr->lock_shared();
        auto rsp = msg->getResponse();
        auto status = msg->getStatus();
        auto serial = msg->getSerial();
        RadioResponseInfo responseInfo{.serial = serial, .error = RadioError::NO_MEMORY};
        RadioError e = RadioError::NONE;
        if ((status != Message::Callback::Status::SUCCESS) ||
            (rsp != rildata::ResponseError_t::NO_ERROR))
        {
            switch (rsp)
            {
            case rildata::ResponseError_t::NOT_SUPPORTED:
                e = RadioError::REQUEST_NOT_SUPPORTED;
                break;
            case rildata::ResponseError_t::INVALID_ARGUMENT:
                e = RadioError::INVALID_ARGUMENTS;
                break;
            case rildata::ResponseError_t::CALL_NOT_AVAILABLE:
                e = RadioError::INVALID_CALL_ID;
                break;
            default:
                e = RadioError::GENERIC_FAILURE;
                break;
            }
        }
        responseInfo = {.serial = serial, .error = e};
        QCRIL_LOG_DEBUG("deactivate data call cb invoked status %d respErr %d", status, rsp);

        static_cast<RadioImpl*>(ri.get())->deactivateDataCallResponse(responseInfo);

        radioServiceRwlockPtr->unlock_shared();
    }
}
#endif
