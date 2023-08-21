/******************************************************************************
#  Copyright (c) 2015-2017, 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/*!
  @file
  qcril_qmi_radio_config_imss.h

  @brief
  Contains imss related radio config req and response handlers

*/


#ifndef DEPRICATED_QCRIL_QMI_RADIO_CONFIG_IMSS_H
#define DEPRICATED_QCRIL_QMI_RADIO_CONFIG_IMSS_H

#include "qcrili.h"
#include "../qcril_qmi_radio_config_imss.h"
#include "qcril_qmi_radio_config_meta.h"

ims_settings_wfc_roaming_enum_v01 qcril_qmi_radio_config_imss_map_radio_config_wifi_roaming_to_ims_wifi_roaming
(
  qcril_qmi_radio_config_imss_voice_over_wifi_roaming_type radio_config_wifi_roaming
);
/*===========================================================================
                          GET REQUEST HANDLERS
===========================================================================*/

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_voip_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_client_provisioning_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_user_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_sms_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_presence_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_vice_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

/*===========================================================================
                          SET REQUEST HANDLERS
===========================================================================*/

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_voip_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_client_provisioning_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_user_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_sms_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_presence_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_vice_config_req_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

/*===========================================================================
                           GET RESPONSE HANDLERS
===========================================================================*/

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_voip_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_extended_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_client_provisioning_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_user_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_sms_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_presence_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_reg_mgr_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_get_qipcall_vice_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

/*===========================================================================
                           SET RESPONSE HANDLERS
===========================================================================*/

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_voip_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_extended_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_client_provisioning_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_user_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_sms_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_presence_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_reg_mgr_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

qcril_qmi_ims_config_error_type qcril_qmi_radio_config_imss_set_qipcall_vice_config_resp_handler
(
  const qcril_qmi_radio_config_params_type *const config_ptr,
  uint16_t req_id
);

#endif /* QCRIL_QMI_RADIO_CONFIG_IMSS_H */
