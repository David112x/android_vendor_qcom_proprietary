/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/Message.h>
#include <framework/Module.h>
#include <framework/AddPendingMessageList.h>
#include <framework/PendingMessageList.h>
#include <framework/QcrilInitMessage.h>
#include <interfaces/lte_direct/QcRilRequestCancelPublishMessage.h>
#include <interfaces/lte_direct/QcRilRequestCancelSubscribeMessage.h>
#include <interfaces/lte_direct/QcRilRequestGetDeviceCapabilityMessage.h>
#include <interfaces/lte_direct/QcRilRequestGetLtedCategoryMessage.h>
#include <interfaces/lte_direct/QcRilRequestGetLtedConfigMessage.h>
#include <interfaces/lte_direct/QcRilRequestGetServiceStatusMessage.h>
#include <interfaces/lte_direct/QcRilRequestLteDirectInitializeMessage.h>
#include <interfaces/lte_direct/QcRilRequestPublishMessage.h>
#include <interfaces/lte_direct/QcRilRequestSetLtedCategoryMessage.h>
#include <interfaces/lte_direct/QcRilRequestSetLtedConfigMessage.h>
#include <interfaces/lte_direct/QcRilRequestSubscribeMessage.h>
#include <interfaces/lte_direct/QcRilRequestTerminateMessage.h>
#include <modules/qmi/QmiAsyncResponseMessage.h>

#include "lte_v01.h"

#define QCRIL_QMI_LTE_DIRECT_DISC_OP_NONE ((uint32_t)0)
#define QCRIL_QMI_LTE_DIRECT_DISC_OP_PUBLISH (((uint32_t)1) << 0)
#define QCRIL_QMI_LTE_DIRECT_DISC_OP_SUBSCRIBE (((uint32_t)1) << 1)

typedef struct qcril_qmi_lte_direct_disc_overview_exp_type {
  char *expression;
  struct qcril_qmi_lte_direct_disc_overview_exp_type *next;
} qcril_qmi_lte_direct_disc_overview_exp_type;

typedef struct qcril_qmi_lte_direct_disc_exec_overview_type {
  char *os_app_id;
  qcril_qmi_lte_direct_disc_overview_exp_type *publish_list;
  qcril_qmi_lte_direct_disc_overview_exp_type *subscribe_list;
  struct qcril_qmi_lte_direct_disc_exec_overview_type *next;
} qcril_qmi_lte_direct_disc_exec_overview_type;

typedef struct {
  qcril_qmi_lte_direct_disc_exec_overview_type *exec_overview_root;
} qcril_qmi_lte_direct_disc_overview_type;

class LteDirectModule : public Module, public AddPendingMessageList {
public:
  LteDirectModule();
  ~LteDirectModule();

  void init();

  bool isReady();
#ifdef QMI_RIL_UTF
  void cleanup();
#endif

private:
  bool mReady = false;

  void handleQmiIndMessage(std::shared_ptr<Message> msg);
  void handleEndpointStatusIndMessage(std::shared_ptr<Message> msg);

  void handleQmiAsyncRespMessage(std::shared_ptr<QmiAsyncResponseMessage> msg);

  void handleQcrilInitMessage(std::shared_ptr<QcrilInitMessage> msg);

  void
  handleQcRilRequestGetLtedConfigMessage(std::shared_ptr<QcRilRequestGetLtedConfigMessage> msg);
  void
  handleQcRilRequestGetLtedCategoryMessage(std::shared_ptr<QcRilRequestGetLtedCategoryMessage> msg);
  void
  handleQcRilRequestSetLtedCategoryMessage(std::shared_ptr<QcRilRequestSetLtedCategoryMessage> msg);
  void
  handleQcRilRequestSetLtedConfigMessage(std::shared_ptr<QcRilRequestSetLtedConfigMessage> msg);

  void handleQcRilRequestLteDirectInitializeMessage(
      std::shared_ptr<QcRilRequestLteDirectInitializeMessage> msg);
  void handleQcRilRequestGetDeviceCapabilityMessage(
      std::shared_ptr<QcRilRequestGetDeviceCapabilityMessage> msg);
  void handleQcRilRequestGetServiceStatusMessage(
      std::shared_ptr<QcRilRequestGetServiceStatusMessage> msg);
  void handleQcRilRequestPublishMessage(std::shared_ptr<QcRilRequestPublishMessage> msg);
  void
  handleQcRilRequestCancelPublishMessage(std::shared_ptr<QcRilRequestCancelPublishMessage> msg);
  void handleQcRilRequestSubscribeMessage(std::shared_ptr<QcRilRequestSubscribeMessage> msg);
  void
  handleQcRilRequestCancelSubscribeMessage(std::shared_ptr<QcRilRequestCancelSubscribeMessage> msg);
  void handleQcRilRequestTerminateMessage(std::shared_ptr<QcRilRequestTerminateMessage> msg);

  // Indication handlers
  void handleLteDiscNotificationInd(qmi_lte_disc_notification_ind_msg_v01 *notif_ind_msg);
  void handleLteDiscBroadcastNotificationInd(
      qmi_lte_disc_broadcast_notification_ind_msg_v01 *notif_ind_msg);
  void
  handleLteDiscMatchNotificationInd(qmi_lte_disc_match_notification_ind_msg_v01 *notif_ind_msg);
  void handleLteDiscSubscriptionInfoInd(qmi_lte_subscription_info_ind_msg_v01 *notif_ind_msg);
  void handleLteDiscPskExpirtedInd(qmi_lte_disc_psk_expired_ind_msg_v01 * /*notif_ind_msg*/);
};

LteDirectModule *getLteDirectModule();
