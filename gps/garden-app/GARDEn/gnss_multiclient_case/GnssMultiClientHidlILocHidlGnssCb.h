/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/
#ifndef GNSS_MULTI_CLIENT_HIDL_ILOCHIDLGNSS_CB_H
#define GNSS_MULTI_CLIENT_HIDL_ILOCHIDLGNSS_CB_H
#include "GnssMultiClientCase.h"
#include "GnssCbBase.h"

#include <vendor/qti/gnss/3.0/ILocHidlGnss.h>
#include <vendor/qti/gnss/3.0/ILocHidlIzatConfig.h>
#include <vendor/qti/gnss/1.1/ILocHidlIzatSubscriptionCallback.h>
using android::hardware::Return;
using android::hardware::Void;
using ::android::hardware::hidl_vec;

using ::vendor::qti::gnss::V3_0::ILocHidlGnss;
using ::vendor::qti::gnss::V3_0::ILocHidlIzatConfigCallback;
using ::vendor::qti::gnss::V1_1::ILocHidlIzatSubscriptionCallback;
using vendor::qti::gnss::V1_0::LocHidlSubscriptionDataItemId;

namespace garden {

class GnssMultiClientHidlILocHidlGnss;

class GnssMultiClientHidlILocHidlGnssCb :
        public ILocHidlIzatConfigCallback,
        public ILocHidlIzatSubscriptionCallback
{
public:
    GnssMultiClientHidlILocHidlGnssCb(GnssMultiClientHidlILocHidlGnss* hidlILocHidlGnss);
    ~GnssMultiClientHidlILocHidlGnssCb();

    //IzatConfigCb
    Return<void> izatConfigCallback(const ::android::hardware::hidl_string& izatConfigContent);

    //Izatsubscriptioncb
    Return<void> requestData(const hidl_vec<LocHidlSubscriptionDataItemId>& list) {return Void();}
    Return<void> updateSubscribe(const hidl_vec<LocHidlSubscriptionDataItemId>& list,
        bool subscribe) { return Void();}
    Return<void> unsubscribeAll() {return Void();}
    Return<void> turnOnModule(LocHidlSubscriptionDataItemId di, int32_t timeout) {return Void();}
    Return<void> turnOffModule(LocHidlSubscriptionDataItemId di) {return Void();}

private:
    GnssMultiClientHidlILocHidlGnss* mLocHidlGnss = nullptr;
};

} //namespace garden

#endif //GNSS_MULTI_CLIENT_HIDL_ILOCHIDLGNSS_CB_H
