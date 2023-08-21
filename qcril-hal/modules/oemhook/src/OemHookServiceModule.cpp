/******************************************************************************
#  Copyright (c) 2018 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#define TAG "OemHookServiceModule"
#include "modules/oemhook/OemHookServiceModule.h"

#undef  TAG
#define TAG "OemHookServiceModule"

OemHookServiceModule::OemHookServiceModule(qcril_instance_id_e_type instance)
        : mInstance(instance) {
    mName = "OemHookServiceModule";

    using std::placeholders::_1;
    mMessageHandler = {
        HANDLER(QcrilInitMessage, OemHookServiceModule::handleQcrilInit),
        HANDLER(QcRilUnsolAdnRecordsOnSimMessage, OemHookServiceModule::handleAdnRecordsOnSimMessage),
        HANDLER(QcRilUnsolAdnInitDoneMessage, OemHookServiceModule::handleAdnInitDoneMessage),
        HANDLER(QcRilUnsolMbnConfigResultMessage, OemHookServiceModule::handleMbnConfigResultMessage),
        HANDLER(QcRilUnsolMbnConfigClearedMessage, OemHookServiceModule::handleMbnConfigClearedMessage),
        HANDLER(QcRilUnsolMbnValidateDumpedMessage, OemHookServiceModule::handleMbnValidateDumpedMessage),
        HANDLER(QcRilUnsolMbnConfigListMessage, OemHookServiceModule::handleMbnConfigListMessage),
        HANDLER(QcRilUnsolMbnValidateConfigMessage, OemHookServiceModule::handleMbnValidateConfigMessage),
        HANDLER(UimSimlockTempUnlockExpireInd, OemHookServiceModule::handleUimSimlockTempUnlockExpireInd),
        HANDLER(UimCardStateChangeInd, OemHookServiceModule::handleUimCardStateChangeInd),
        HANDLER(UimSlotStatusInd, OemHookServiceModule::handleUimSlotStatusInd),
        HANDLER(UimSimRefreshIndication, OemHookServiceModule::handleUimSimRefreshIndication),
        HANDLER(UimVoltageStatusInd, OemHookServiceModule::handleUimVoltageStatusInd),
        HANDLER(UimRmtRemoteSimStatusIndMsg, OemHookServiceModule::handleUimRmtRemoteSimStatusIndMsg),
        HANDLER(QcRilUnsolDtmfMessage, OemHookServiceModule::handleQcRilUnsolDtmfMessage),
        HANDLER(QcRilUnsolExtBurstIntlMessage, OemHookServiceModule::handleQcRilUnsolExtBurstIntlMessage),
        HANDLER(QcRilUnsolNssReleaseMessage, OemHookServiceModule::handleQcRilUnsolNssReleaseMessage),
        HANDLER(QcRilUnsolSuppSvcErrorCodeMessage, OemHookServiceModule::handleQcRilUnsolSuppSvcErrorCodeMessage),
        HANDLER(QcRilUnsolSpeechCodecInfoMessage, OemHookServiceModule::handleQcRilUnsolSpeechCodecInfoMessage),

#ifdef FEATURE_QCRIL_LTE_DIRECT
        HANDLER(QcRilUnsolAuthorizationResultMessage, OemHookServiceModule::handleQcRilUnsolAuthorizationResultMessage),
        HANDLER(QcRilUnsolDeviceCapabilityChangedMessage, OemHookServiceModule::handleQcRilUnsolDeviceCapabilityChangedMessage),
        HANDLER(QcRilUnsolExpressionStatusMessage, OemHookServiceModule::handleQcRilUnsolExpressionStatusMessage),
        HANDLER(QcRilUnsolMatchEventMessage, OemHookServiceModule::handleQcRilUnsolMatchEventMessage),
        HANDLER(QcRilUnsolPskExpirtedMessage, OemHookServiceModule::handleQcRilUnsolPskExpirtedMessage),
        HANDLER(QcRilUnsolReceptionStatusMessage, OemHookServiceModule::handleQcRilUnsolReceptionStatusMessage),
        HANDLER(QcRilUnsolServiceStatusMessage, OemHookServiceModule::handleQcRilUnsolServiceStatusMessage),
        HANDLER(QcRilUnsolTransmissionStatusMessage, OemHookServiceModule::handleQcRilUnsolTransmissionStatusMessage),
#endif
    };
}

void OemHookServiceModule::handleQcrilInit(std::shared_ptr<QcrilInitMessage> msg) {
    QCRIL_LOG_INFO("Handling QcrilInitMessage %s", msg->dump().c_str());
}

sp<OemHookImpl> OemHookServiceModule::getOemHookImpl()
{
    auto rs = getOemHookService(mInstance);
    return rs;
}

void OemHookServiceModule::handleAdnRecordsOnSimMessage(
        std::shared_ptr<QcRilUnsolAdnRecordsOnSimMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendAdnRecords(msg);
    }
}

void OemHookServiceModule::handleAdnInitDoneMessage(
        std::shared_ptr<QcRilUnsolAdnInitDoneMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendAdnInitDone(msg);
    }
}

void OemHookServiceModule::handleQcRilUnsolDtmfMessage(
        std::shared_ptr<QcRilUnsolDtmfMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
      switch (msg->getDtmfEvent()) {
        case qcril::interfaces::DtmfEvent::FWD_BURST:
          ri->sendUnsolCdmaBurstDtmf(msg);
          break;
        case qcril::interfaces::DtmfEvent::FWD_START_CONT:
          ri->sendUnsolCdmaContDtmfStart(msg);
          break;
        case qcril::interfaces::DtmfEvent::FWD_STOP_CONT:
          ri->sendUnsolCdmaContDtmfStop(msg);
          break;
        case qcril::interfaces::DtmfEvent::UNKNOWN:
        default:
          break;
      }
    }
}

void OemHookServiceModule::handleQcRilUnsolExtBurstIntlMessage(
        std::shared_ptr<QcRilUnsolExtBurstIntlMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendUnsolExtendedDbmIntl(msg);
    }
}

void OemHookServiceModule::handleQcRilUnsolNssReleaseMessage(
        std::shared_ptr<QcRilUnsolNssReleaseMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendUnsolNssRelease(msg);
    }
}

void OemHookServiceModule::handleQcRilUnsolSuppSvcErrorCodeMessage(
        std::shared_ptr<QcRilUnsolSuppSvcErrorCodeMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendUnsolSsErrorCode(msg);
    }
}

void OemHookServiceModule::handleQcRilUnsolSpeechCodecInfoMessage(
        std::shared_ptr<QcRilUnsolSpeechCodecInfoMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendUnsolSpeechCodecInfo(msg);
    }
}

void OemHookServiceModule::handleMbnConfigResultMessage(
        std::shared_ptr<QcRilUnsolMbnConfigResultMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendMbnConfigResult(msg);
    }
}

void OemHookServiceModule::handleMbnConfigClearedMessage(
        std::shared_ptr<QcRilUnsolMbnConfigClearedMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendMbnConfigCleared(msg);
    }
}

void OemHookServiceModule::handleMbnValidateDumpedMessage(
        std::shared_ptr<QcRilUnsolMbnValidateDumpedMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendMbnValidateDumped(msg);
    }
}

void OemHookServiceModule::handleMbnConfigListMessage(
        std::shared_ptr<QcRilUnsolMbnConfigListMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendMbnConfigList(msg);
    }
}

void OemHookServiceModule::handleMbnValidateConfigMessage(
        std::shared_ptr<QcRilUnsolMbnValidateConfigMessage> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->sendMbnValidateConfig(msg);
    }
}

void OemHookServiceModule::handleUimSimlockTempUnlockExpireInd(
        std::shared_ptr<UimSimlockTempUnlockExpireInd> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->uimSimlockTempUnlockExpireInd(msg);
    }
}

void OemHookServiceModule::handleUimCardStateChangeInd(
        std::shared_ptr<UimCardStateChangeInd> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->uimCardStateChangeInd(msg);
    }
}

void OemHookServiceModule::handleUimSlotStatusInd(
        std::shared_ptr<UimSlotStatusInd> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->uimSlotStatusInd(msg);
    }
}

void OemHookServiceModule::handleUimSimRefreshIndication(
        std::shared_ptr<UimSimRefreshIndication> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->uimSimRefreshIndication(msg);
    }
}

void OemHookServiceModule::handleUimVoltageStatusInd(
        std::shared_ptr<UimVoltageStatusInd> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->uimVoltageStatusInd(msg);
    }
}

void OemHookServiceModule::handleUimRmtRemoteSimStatusIndMsg(
        std::shared_ptr<UimRmtRemoteSimStatusIndMsg> msg) {
    QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());

    auto ri = getOemHookImpl();
    if (ri) {
        ri->uimRmtRemoteSimStatusIndMsg(msg);
    }
}

#ifdef FEATURE_QCRIL_LTE_DIRECT
void OemHookServiceModule::handleQcRilUnsolAuthorizationResultMessage(
    std::shared_ptr<QcRilUnsolAuthorizationResultMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolAuthorizationResult(msg);
  }
}

void OemHookServiceModule::handleQcRilUnsolDeviceCapabilityChangedMessage(
    std::shared_ptr<QcRilUnsolDeviceCapabilityChangedMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolDeviceCapabilityChanged(msg);
  }
}

void OemHookServiceModule::handleQcRilUnsolExpressionStatusMessage(
    std::shared_ptr<QcRilUnsolExpressionStatusMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolExpressionStatus(msg);
  }
}

void OemHookServiceModule::handleQcRilUnsolMatchEventMessage(
    std::shared_ptr<QcRilUnsolMatchEventMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolMatchEvent(msg);
  }
}

void OemHookServiceModule::handleQcRilUnsolPskExpirtedMessage(
    std::shared_ptr<QcRilUnsolPskExpirtedMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolPskExpirted(msg);
  }
}

void OemHookServiceModule::handleQcRilUnsolReceptionStatusMessage(
    std::shared_ptr<QcRilUnsolReceptionStatusMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolReceptionStatus(msg);
  }
}

void OemHookServiceModule::handleQcRilUnsolServiceStatusMessage(
    std::shared_ptr<QcRilUnsolServiceStatusMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolServiceStatus(msg);
  }
}

void OemHookServiceModule::handleQcRilUnsolTransmissionStatusMessage(
    std::shared_ptr<QcRilUnsolTransmissionStatusMessage> msg) {
  QCRIL_LOG_INFO("Handling %s", msg->dump().c_str());
  auto ri = getOemHookImpl();
  if (ri) {
    ri->sendLteDirectUnsolTransmissionStatus(msg);
  }
}
#endif
