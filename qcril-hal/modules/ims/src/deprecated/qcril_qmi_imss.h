/******************************************************************************
#  Copyright (c) 2013-2015, 2017, 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/******************************************************************************
  @file    qcril_qmi_imss.h
  @brief   qcril qmi - IMS Setting

  DESCRIPTION
    Handles RIL requests, Callbacks, indications for QMI IMS Setting.

******************************************************************************/

#ifndef QCRIL_QMI_IMSS_DEPRICATED_H
#define QCRIL_QMI_IMSS_DEPRICATED_H

#include <memory>
#include "interfaces/ims/QcRilRequestImsRegistrationChangeMessage.h"
#include "interfaces/ims/QcRilRequestImsSetServiceStatusMessage.h"
#include "interfaces/ims/QcRilRequestImsSetConfigMessage.h"
#include "interfaces/ims/QcRilRequestImsGetConfigMessage.h"

void qcril_qmi_imss_get_client_provisioning_config();

void qcril_qmi_imss_client_provisioning_config_ind_hdlr(void *ind_data_ptr);

void qcril_qmi_imss_config_resp_cb
(
   unsigned int                 msg_id,
   std::shared_ptr<void>        resp_c_struct,
   unsigned int                 resp_c_struct_len,
   void                        *resp_cb_data,
   qmi_client_error_type        transp_err
);

void qcril_qmi_imss_unsol_ind_cb_helper
(
 unsigned int   msg_id,
 unsigned char *decoded_payload,
 uint32_t       decoded_payload_len
);

void qcril_qmi_imss_request_set_ims_registration(
    std::shared_ptr<QcRilRequestImsRegistrationChangeMessage> msg);
void qcril_qmi_imss_request_set_ims_srv_status(
    std::shared_ptr<QcRilRequestImsSetServiceStatusMessage> msg);
void qcril_qmi_imss_request_set_ims_config(
    std::shared_ptr<QcRilRequestImsSetConfigMessage> msg);
void qcril_qmi_imss_request_get_ims_config(
    std::shared_ptr<QcRilRequestImsGetConfigMessage> msg);

#endif
