/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <vector>
#include <binder/IBinder.h>
#include <utils/String8.h>

#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_death_recipient;
using ::android::sp;

using namespace ::android::hardware::automotive::vehicle::V2_0;

namespace android {

class AutoPower : public IVehicleCallback
{
    /*Overrides*/
    Return<void> onPropertyEvent(const hidl_vec<VehiclePropValue>& propValues) override;
    Return<void> onPropertySet(const VehiclePropValue& propValue) override;
    Return<void> onPropertySetError(StatusCode errorCode, int32_t propId, int32_t areaId) override;

    void handleVehicleHalDeath();

    class VehicleHalDeathRecipient : public hidl_death_recipient
    {
        public:
            VehicleHalDeathRecipient(const sp<AutoPower> autopower) :
                    mAutoPower(autopower) {}
            virtual void serviceDied(uint64_t cookie,
                    const wp<hidl::base::V1_0::IBase>& who);
        private:
            sp<AutoPower> mAutoPower;
    };

public:
    AutoPower();
    virtual ~AutoPower();

private:
    status_t initVehicle();

    sp<IVehicle> mVehicle;
    sp<VehicleHalDeathRecipient> mVehicleHalDeathRecipient;
};

}
