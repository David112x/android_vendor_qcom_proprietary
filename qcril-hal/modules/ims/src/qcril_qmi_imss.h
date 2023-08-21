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

#ifndef QCRIL_QMI_IMSS_H
#define QCRIL_QMI_IMSS_H

#include "qmi_client.h"
#include "qmi.h"
#include "qcril_reqlist.h"
#include "qcril_qmi_radio_config_meta.h"
#include "qcril_qmi_imsa.h"
#include "ip_multimedia_subsystem_settings_v01.h"
#include "network_access_service_v01.h"

#ifdef __cplusplus
#include <memory>
#include "modules/ims/ImsServiceStatusInd.h"
#include "modules/ims/ImsWfcSettingsStatusInd.h"
#include "interfaces/ims/QcRilRequestImsRegistrationChangeMessage.h"
#include "interfaces/ims/QcRilRequestImsSetServiceStatusMessage.h"
#include "interfaces/ims/QcRilRequestImsSetConfigMessage.h"
#include "interfaces/ims/QcRilRequestImsGetConfigMessage.h"
#include "interfaces/ims/QcRilRequestImsRegisterMultiIdentityMessage.h"
#include <interfaces/ims/QcRilUnsolImsWfcRoamingConfigIndication.h>
#ifdef FEATURE_SUPPORT_IMSS_DEPRECATED
#include "deprecated/qcril_qmi_imss.h"
#endif
#endif


#define QMI_RIL_DISABLE_MODEM_CONFIG "persist.vendor.radio.disable_modem_cfg"
#define QMI_RIL_DISABLE_MODEM_CONFIG_LENGTH (4)

#define QMI_RIL_IMSS_VOICE_DOMAIN_PREF_ON_IMS_TOGGLE "persist.vendor.radio.vdp_on_ims_cap"

//===========================================================================
//                    INTERNAL DEFINITIONS AND TYPES
//===========================================================================
typedef enum
{
  QCRIL_IMS_SETTING_WFC_NOT_SUPPORTED = 0,
  QCRIL_IMS_SETTING_WFC_OFF = 1,
  QCRIL_IMS_SETTING_WFC_ON = 2
} qcril_ims_setting_wfc_status_type;

typedef enum
{
  QCRIL_IMS_SETTING_WFC_WLAN_PREFERRED = 0,
  QCRIL_IMS_SETTING_WFC_WLAN_ONLY = 1,
  QCRIL_IMS_SETTING_WFC_CELLULAR_PREFERRED = 2,
  QCRIL_IMS_SETTING_WFC_CELLULAR_ONLY = 3,
  QCRIL_IMS_SETTING_WFC_IMS_PREFERRED = 4,
  QCRIL_IMS_SETTING_WFC_PREF_NONE = 5,
} qcril_ims_setting_wfc_preference_type;

#ifdef __cplusplus
struct ims_cached_info_type
{
  qtimutex::QtiSharedMutex imss_info_lock_mutex;

  uint8_t wifi_calling_enabled_valid;
  qcril_ims_setting_wfc_status_type wifi_calling_enabled;

  uint8_t call_mode_preference_valid;
  qcril_ims_setting_wfc_preference_type call_mode_preference;
  uint8_t call_mode_roam_preference_valid;
  qcril_ims_setting_wfc_preference_type call_mode_roam_preference;

  uint8_t service_mask_by_network_enabled_valid;
  ims_settings_omadm_services_type_mask_v01 service_mask_by_network_enabled;
};
#endif


//===========================================================================
//                     IMSS Utility functions
//===========================================================================
void qcril_qmi_imss_pre_init(void);

void qcril_qmi_imss_init(void);

void qcril_qmi_imss_cleanup(void);

void qcril_qmi_imss_info_lock(void);

void qcril_qmi_imss_info_unlock(void);

void qcril_qmi_ims_send_unsol_wfc_roaming_config();

#ifdef __cplusplus
void qcril_qmi_imss_command_cb
(
   unsigned int                 msg_id,
   std::shared_ptr<void>        resp_c_struct,
   unsigned int                 resp_c_struct_len,
   void                        *resp_cb_data,
   qmi_client_error_type        transp_err
);

void qcril_qmi_imss_config_resp_cb_v02
(
   unsigned int                 msg_id,
   std::shared_ptr<void>        resp_c_struct,
   unsigned int                 resp_c_struct_len,
   void                        *resp_cb_data,
   qmi_client_error_type        transp_err
);

void qcril_qmi_imss_register_multi_identity_lines_resp_hdlr
(
   unsigned int                 msg_id,
   std::shared_ptr<void>        resp_c_struct,
   unsigned int                 resp_c_struct_len,
   void                        *resp_cb_data,
   qmi_client_error_type        transp_err
);

void qcril_qmi_imss_request_register_multi_identity_lines
(
    std::shared_ptr<QcRilRequestImsRegisterMultiIdentityMessage> msg
);
ims_settings_uri_line_type_v01 converToQmiMultiIdentityLineType(int lineType);

#endif

void qcril_qmi_imss_handle_ims_set_service_status_event_resp
(
 uint16 req_id,
 RIL_Errno ril_err
);

void qcril_qmi_imss_dispatch_config_response_helper
(
   const qcril_qmi_radio_config_resp_data_type *const resp_ptr,
   int config_item,
   unsigned long msg_id,
   uint16_t req_id
);

uint8_t qcril_qmi_imss_is_modem_supports_wfc_preferred_ims(void);

void qcril_qmi_imss_set_ims_config_log_and_send_response
(
   uint16_t req_id,
   const qcril_qmi_radio_config_params_type *const config_params,
   qcril_qmi_ims_config_error_type error,
   qcril_qmi_ims_config_settings_resp_type settings_resp
);

void qcril_qmi_imss_get_ims_config_log_and_send_response
(
   uint16_t req_id,
   const qcril_qmi_radio_config_params_type *const config_params,
   qcril_qmi_ims_config_error_type error,
   qcril_qmi_ims_config_settings_resp_type settings_resp
);

void qcril_qmi_imss_update_wifi_pref_from_ind_to_qcril_data();

#ifdef __cplusplus
void qcril_qmi_imss_broadcast_wfc_settings
(
 const struct ims_cached_info_type& ims_info
);
#endif

qcril_ims_setting_wfc_status_type qcril_qmi_imss_convert_imss_to_ril_wfc_status
(
  ims_settings_wfc_status_enum_v01 wfc_status
);

qcril_ims_setting_wfc_preference_type qcril_qmi_imss_convert_imss_to_ril_wfc_preference
(
  ims_settings_wfc_preference_v01 wfc_preference
);

#endif
