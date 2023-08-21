/******************************************************************************
#  Copyright (c) 2015, 2017 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
/******************************************************************************
  @file    qcril_qmi_radio_config_data.c
  @brief   qcril qmi - radio config data handlers

  DESCRIPTION
    Handles data related radio config req and response handlers

******************************************************************************/
#include "qcril_qmi_radio_config_data.h"
#include "qcrili.h"
//----PLACEHOLDERS FOR NOW-----
/* **needs to be discussed**
 * Since this is a sync request, it will call the send response handler directly
 * which will have to take care of mapping the req-resp config items
 */
RIL_Errno qcril_qmi_radio_config_data_set_quality_measurement_req_handler
(
  const void *const config_ptr,
  uint16_t req_id
)
{
    QCRIL_NOTUSED(config_ptr);
    (void)req_id;
    return RIL_E_GENERIC_FAILURE;
}

/* **needs to be discussed**
 * Should be called from the req handler..!need to be discussed!
 */
RIL_Errno qcril_qmi_radio_config_data_set_quality_measurement_resp_handler
(
  const void *const config_ptr,
  uint16_t req_id
)
{
    QCRIL_NOTUSED(config_ptr);
    (void)req_id;
    return RIL_E_GENERIC_FAILURE;
}
