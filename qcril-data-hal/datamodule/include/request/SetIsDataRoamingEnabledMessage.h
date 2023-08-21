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

class SetIsDataRoamingEnabledMessage : public SolicitedMessage<RIL_Errno>,
                                       public add_message_id<SetIsDataRoamingEnabledMessage> {
    private:
    legacy_request_payload params;

    public:
    bool is_data_roaming_enabled;
    static constexpr const char *MESSAGE_NAME = "QCRIL_DATA_SET_IS_DATA_ROAMING_ENABLED";
    SetIsDataRoamingEnabledMessage() = delete;
    inline SetIsDataRoamingEnabledMessage( const qcril_request_params_type &p ):
              SolicitedMessage<RIL_Errno>(get_class_message_id()),params(p) {
        mName = MESSAGE_NAME;
        is_data_roaming_enabled = *((uint8_t *)p.data);
    }
    qcril_request_params_type &get_params() {
          return params.get_params();
    }
    ~SetIsDataRoamingEnabledMessage() {
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

class SetIsDataRoamingEnabledMessage : public SolicitedMessage<RIL_Errno>,
                                       public add_message_id<SetIsDataRoamingEnabledMessage> {
    private:
    boolean mDataRoamingEnabled;

    public:
    static constexpr const char *MESSAGE_NAME = "QCRIL_DATA_SET_IS_DATA_ROAMING_ENABLED";
    SetIsDataRoamingEnabledMessage() = delete;
    ~SetIsDataRoamingEnabledMessage() = default;
    string dump(){return mName;}
    inline SetIsDataRoamingEnabledMessage( boolean isDataRoamingEnabled ):
              SolicitedMessage<RIL_Errno>(get_class_message_id()),mDataRoamingEnabled(isDataRoamingEnabled) {
        mName = MESSAGE_NAME;
    }
    boolean getDataRoamingState() {
          return mDataRoamingEnabled;
    }

};
#endif

}

