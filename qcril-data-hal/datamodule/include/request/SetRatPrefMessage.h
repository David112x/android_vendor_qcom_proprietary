/**
* Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
**/

#pragma once
#include "framework/SolicitedMessage.h"
#include "framework/GenericCallback.h"
#include "DataCommon.h"
#include "framework/add_message_id.h"
#include "framework/message_translator.h"

namespace rildata {

enum class RatPreference {
    Inactive = 0,
    CellularOnly,
    WifiOnly,
    CellularPreferred,
    WifiPreferred,
    ImsPreferred,
};

#define QCRIL_DATA_USE_DSD_RAT_PREF 1
class SetRatPrefMessage : public SolicitedMessage<RIL_Errno>,public add_message_id<SetRatPrefMessage> {
    private:
    RatPreference ratPrefType;
    RatPreference roamingRatPref;

    public:
    static constexpr const char *MESSAGE_NAME = "QCRIL_DATA_SET_RAT_PREF";
    SetRatPrefMessage() = delete;

#ifdef QCRIL_DATA_USE_DSD_RAT_PREF
    inline SetRatPrefMessage( RatPreference rat_pref ):SolicitedMessage<RIL_Errno>(get_class_message_id()) {
        mName = MESSAGE_NAME;
        ratPrefType = rat_pref;
        roamingRatPref = RatPreference::Inactive; //Default value
    }
#endif
    inline SetRatPrefMessage( RatPreference rat_pref, RatPreference roaming_rat_pref ):SolicitedMessage<RIL_Errno>(get_class_message_id()) {
        mName = MESSAGE_NAME;
        ratPrefType = rat_pref;
        roamingRatPref = roaming_rat_pref;
    }
    ~SetRatPrefMessage();

    RatPreference getRatPreference();

    RatPreference getRoamingRatPreference();

    string dump();
};
}
