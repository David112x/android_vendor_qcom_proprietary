/******************************************************************************
@file    uim_lpa_service.h
@brief   qcril uim lpa service

DESCRIPTION
Implements the server side of the IUimLpa interface.
Handles RIL requests and responses and indications to be received
and sent to client respectively

---------------------------------------------------------------------------

Copyright (c) 2017 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
---------------------------------------------------------------------------
******************************************************************************/
#ifndef VENDOR_QTI_HARDWARE_RADIO_UIM_LPA_V1_0_H
#define VENDOR_QTI_HARDWARE_RADIO_UIM_LPA_V1_0_H

#include <vendor/qti/hardware/radio/lpa/1.0/IUimLpa.h>
#include <vendor/qti/hardware/radio/lpa/1.1/IUimLpa.h>
#include <vendor/qti/hardware/radio/lpa/1.1/types.h>
#include <vendor/qti/hardware/radio/lpa/1.1/IUimLpaIndication.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include "modules/lpa/lpa_service_types.h"

class LpaModule;

using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::sp;
using ::android::wp;

namespace vendor {
namespace qti {
namespace hardware {
namespace radio {
namespace lpa {
namespace V1_0 {
namespace implementation {

class UimLpaImpl : public V1_1::IUimLpa, public hidl_death_recipient {
  sp<V1_0::IUimLpaResponse> mResponseCb;
  sp<V1_0::IUimLpaIndication> mIndicationCb;
  sp<V1_1::IUimLpaIndication> mIndicationCb_1_1;
  int mInstanceId;
  qtimutex::QtiSharedMutex mCallbackLock;
  LpaModule * mModule;

  sp<V1_0::IUimLpaResponse> getResponseCallback();
  sp<V1_0::IUimLpaIndication> getIndicationCallback();
  sp<V1_1::IUimLpaIndication> getIndicationCallback_1_1();

  void clearCallbacks();
  // Functions from hidl_death_recipient
  void serviceDied( uint64_t, const ::android::wp<::android::hidl::base::V1_0::IBase> &);

  // Methods from IUimLpaResponse
  Return<void> setCallback(const sp<V1_0::IUimLpaResponse>& responseCallback, const sp<IUimLpaIndication>& indicationCallback);
  Return<void> UimLpaUserRequest(int32_t token, const V1_0::UimLpaUserReq& userReq);
  Return<void> UimLpaUserRequest_1_1(int32_t token, const V1_1::UimLpaUserReq& userReq);
  Return<void> UimLpaHttpTxnCompletedRequest(int32_t token, V1_0::UimLpaResult result, const hidl_vec<uint8_t>& responsePayload, const hidl_vec<V1_0::UimLpaHttpCustomHeader>& customHeaders);

public:
  UimLpaImpl(LpaModule * module);
  void resetIndicationCallback();
  void resetResponseCallback();
  void setInstanceId(int instanceId);
  void uimLpaHttpTxnCompletedResponse(int32_t token, lpa_service_result_type result);
  void uimLpaUserResponse(int32_t token, lpa_service_user_resp_type * user_resp);
  void uimLpaAddProfileProgressInd(lpa_service_add_profile_progress_ind_type * progInd);
  void uimLpaHttpTxnIndication(lpa_service_http_transaction_ind_type * txnInd);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace lpa
}  // namespace radio
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

#endif  // VENDOR_QTI_HARDWARE_RADIO_UIM_LPA_V1_0_H
