/******************************************************************************
#  Copyright (c) 2015, 2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/*!
  @file
  qcril_qmi_radio_config_misc.h

  @brief
  Contains miscellaneous functions/structures needed for eadio config

*/

#ifndef QCRIL_QMI_RADIO_CONFIG_MISC_H
#define QCRIL_QMI_RADIO_CONFIG_MISC_H

#include "qcrili.h"
#include "telephony/ril.h"
#include "qcril_qmi_radio_config_item.h"
#include "qcril_qmi_radio_config_imss.h"
#ifdef FEATURE_QCRIL_RADIO_CONFIG_SOCKET
#include "radio_config_interface.pb.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FEATURE_QCRIL_RADIO_CONFIG_SOCKET

RIL_Token qcril_qmi_radio_config_convert_to_ril_token(uint32_t radio_config_token);

uint32_t qcril_qmi_radio_config_convert_ril_token_to_send(RIL_Token token);

qcril_qmi_radio_config_item_type qcril_qmi_radio_config_map_socket_item_to_config_item
(
   com_qualcomm_qti_radioconfiginterface_ConfigItem config_item
);

com_qualcomm_qti_radioconfiginterface_ConfigItem qcril_qmi_radio_config_map_config_item_to_socket_item
(
   qcril_qmi_radio_config_item_type config_item
);

#endif

#ifdef __cplusplus
}
#endif

#endif
