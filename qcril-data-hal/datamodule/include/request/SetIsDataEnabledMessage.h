/**
* Copyright (c) 2017 , 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
**/

#pragma once
#include "framework/SolicitedMessage.h"
#include "framework/GenericCallback.h"
#include "framework/add_message_id.h"
#include "DataCommon.h"

#ifndef RIL_FOR_LOW_RAM
#include "framework/message_translator.h"
#include <modules/android/RilRequestMessage.h>
#endif

namespace rildata {

#ifndef RIL_FOR_LOW_RAM

class SetIsDataEnabledMessage : public SolicitedMessage<RIL_Errno>,
                                public add_message_id<SetIsDataEnabledMessage> {
    private:
    legacy_request_payload params;

    public:
    bool is_data_enabled;
    static constexpr const char *MESSAGE_NAME = "QCRIL_DATA_SET_IS_DATA_ENABLED";
    SetIsDataEnabledMessage()= delete;
    inline SetIsDataEnabledMessage( const qcril_request_params_type &p ):
               SolicitedMessage<RIL_Errno>(get_class_message_id()),params(p) {
      mName = MESSAGE_NAME;
      is_data_enabled = *((uint8_t *)p.data);
    }
    qcril_request_params_type &get_params() {
          return params.get_params();
    }
    ~SetIsDataEnabledMessage() {
      if (mCallback) {
        delete mCallback;
        mCallback = nullptr;
      }
    }
    string dump() {
      return mName + "{ }";
    }
};

#else

class SetIsDataEnabledMessage : public SolicitedMessage<RIL_Errno>,
                                public add_message_id<SetIsDataEnabledMessage> {
    private:
    bool mDataEnabled;

    public:
    static constexpr const char *MESSAGE_NAME = "QCRIL_DATA_SET_IS_DATA_ENABLED";
    SetIsDataEnabledMessage()= delete;
    ~SetIsDataEnabledMessage() = default;
    string dump(){return mName;}
    inline SetIsDataEnabledMessage( bool dataEnabled):
               SolicitedMessage<RIL_Errno>(get_class_message_id()),mDataEnabled(dataEnabled) {
      mName = MESSAGE_NAME;
    }
    boolean getDataState() {
          return mDataEnabled;
    }
};
#endif
}
