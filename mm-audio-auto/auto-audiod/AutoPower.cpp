/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#define LOG_TAG "AutoPower"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0

#include <vhal_v2_0/VehicleUtils.h>
#include "android-base/macros.h"

#include "AutoPower.h"
#include "auto_audio_ext.h"

// Power state report property
#define AP_POWER_STATE_REPORT           (toInt(VehicleProperty::AP_POWER_STATE_REPORT))

// Power state report
#define REPORT_WAIT_FOR_VHAL            (toInt(VehicleApPowerStateReport::WAIT_FOR_VHAL))
#define REPORT_DEEP_SLEEP_ENTRY         (toInt(VehicleApPowerStateReport::DEEP_SLEEP_ENTRY))
#define REPORT_DEEP_SLEEP_EXIT          (toInt(VehicleApPowerStateReport::DEEP_SLEEP_EXIT))
#define REPORT_SHUTDOWN_POSTPONE        (toInt(VehicleApPowerStateReport::SHUTDOWN_POSTPONE))
#define REPORT_SHUTDOWN_START           (toInt(VehicleApPowerStateReport::SHUTDOWN_START))
#define REPORT_ON                       (toInt(VehicleApPowerStateReport::ON))
#define REPORT_SHUTDOWN_PREPARE         (toInt(VehicleApPowerStateReport::SHUTDOWN_PREPARE))
#define REPORT_SHUTDOWN_CANCELLED       (toInt(VehicleApPowerStateReport::SHUTDOWN_CANCELLED))

#define MAX_RETRY_TIME                  100
#define VEHICLE_INIT_SLEEP_WAIT         100 /* 100 ms */

static SubscribeOptions reqVehicleProperties[] = {
    {
        .propId = AP_POWER_STATE_REPORT,
        .flags = SubscribeFlags::EVENTS_FROM_ANDROID
    },
};

namespace android {

    AutoPower::AutoPower() :
            mVehicleHalDeathRecipient(new VehicleHalDeathRecipient(this)) {
        int retryCount = 0;
        do {
            if (initVehicle() == OK) {
                ALOGI("Init vehicle OK");
                break;
            } else {
                if (++retryCount <= MAX_RETRY_TIME) {
                    ALOGE("Sleeping for 100 ms");
                    usleep(VEHICLE_INIT_SLEEP_WAIT*1000);
                } else {
                    ALOGE("Init vehicle fail");
                    break;
                }
            }
        } while (1);
    }

    AutoPower::~AutoPower() {
    }

    status_t AutoPower::initVehicle()
    {
        StatusCode status;
        hidl_vec<SubscribeOptions> options;

        if (mVehicle == 0) {
            mVehicle = IVehicle::getService();
            if (mVehicle != 0) {
                mVehicle->linkToDeath(mVehicleHalDeathRecipient, 0 /*cookie*/);
            } else {
                ALOGE("Failed to obtain IVehicle service");
                return FAILED_TRANSACTION;
            }

            options.setToExternal(reqVehicleProperties, arraysize(reqVehicleProperties));
            status = mVehicle->subscribe(this, options);
            if (status != StatusCode::OK) {
                ALOGE("%s: Subscription to vehicle notifications failed with status %d",
                    __func__, status);
                return FAILED_TRANSACTION;
            }
            ALOGD("%s: Subscription to vehicle notifications successful", __func__);
        }

        return OK;
    }

    Return<void> AutoPower::onPropertyEvent(const hidl_vec <VehiclePropValue> & propValues) {
        ALOGD("%s: Vehicle Property Size %d", __func__, (int)propValues.size());
        return Return<void>();
    }

    Return<void> AutoPower::onPropertySet(const VehiclePropValue & propValue) {

        int32_t property = propValue.prop;
        int32_t state = propValue.value.int32Values[0];
        int32_t param = propValue.value.int32Values[1];

        ALOGV("%s: prop %d, state %d, param %d", __func__, property, state, param);

        if (property == AP_POWER_STATE_REPORT)
        {
            switch (state)
            {
            case REPORT_DEEP_SLEEP_ENTRY:
                ALOGD("%s: DEEP_SLEEP_ENTRY", __func__);
                break;
            case REPORT_DEEP_SLEEP_EXIT:
                ALOGD("%s: DEEP_SLEEP_EXIT", __func__);
                auto_audio_ext_enable_hostless_all();
                break;
            case REPORT_SHUTDOWN_POSTPONE:
                ALOGD("%s: SHUTDOWN_POSTPONE", __func__);
                auto_audio_ext_disable_hostless_all();
                break;
            default:
                ALOGW("%s: Un-handled state %d", __func__, state);
                break;
            }
        } else {
            ALOGW("%s: Unknown prop %d", __func__, property);
        }

        return Return<void>();
    }

    Return<void> AutoPower::onPropertySetError(StatusCode errorCode,
                                    int32_t    propId,
                                    int32_t    areaId) {
        // We don't set values, so we don't listen for set errors
        ALOGD("%s: Error Code: %d, Property ID: 0x%x, Area ID: %d",
            __func__, errorCode, propId, areaId);
        return Return<void>();
    }

    void AutoPower::handleVehicleHalDeath()
    {
        mVehicle->unlinkToDeath(mVehicleHalDeathRecipient);
        mVehicle = NULL;
        int retryCount = 0;

        ALOGI("%s: Reconnecting to IVehicle", __func__);
        do {
            if (initVehicle() == OK) {
                ALOGI("Init vehicle OK");
                break;
            } else {
                if (++retryCount <= MAX_RETRY_TIME) {
                    ALOGE("Sleeping for 100 ms");
                    usleep(VEHICLE_INIT_SLEEP_WAIT*1000);
                } else {
                    ALOGE("Init vehicle fail");
                    break;
                }
            }
        } while (1);
        ALOGI("%s: IVehicle Reconnected", __func__);
    }

    // ----------------------------------------------------------------------------

    void AutoPower::VehicleHalDeathRecipient::serviceDied(uint64_t cookie __unused,
            const wp<hidl::base::V1_0::IBase>& who __unused) {

        ALOGI("IVehicle Died");
        mAutoPower->handleVehicleHalDeath();
    }
}
