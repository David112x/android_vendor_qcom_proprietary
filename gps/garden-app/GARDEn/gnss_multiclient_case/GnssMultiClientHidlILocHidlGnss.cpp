/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
  Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
===========================================================================*/

#define LOG_NDEBUG 0
#define LOG_TAG "GARDEN_GMCC_ILocHidlGnss"

#include "GnssMultiClientHidlILocHidlGnss.h"
#include "GnssMultiClientCaseUtils.h"

#include <android/hardware/gnss/1.0/types.h>

using ::vendor::qti::gnss::V3_0::ILocHidlGnss;

namespace garden {

#define getUserInputEnterToContinue GmccUtils::get().getUserInputEnterToContinue
#define getUserInputInt GmccUtils::get().getUserInputInt
#define getUserInputDouble GmccUtils::get().getUserInputDouble
#define getUserInputMask64Bit GmccUtils::get().getUserInputMask64Bit
#define getUserInputString GmccUtils::get().getUserInputString
#define getUserInputYesNo GmccUtils::get().getUserInputYesNo
#define getUserInputSessionMode GmccUtils::get().getUserInputSessionMode
#define getUserInputTrackingOptions GmccUtils::get().getUserInputTrackingOptions
#define getUserInputLocClientIndex GmccUtils::get().getUserInputLocClientIndex
#define getUserInputGnssConfig GmccUtils::get().getUserInputGnssConfig
#define getUserInputGnssConfigFlagsMask GmccUtils::get().getUserInputGnssConfigFlagsMask
#define getUserInputGnssConfigBlacklistSvId GmccUtils::get().getUserInputGnssConfigBlacklistSvId
#define getUserInputGnssSvIdSource GmccUtils::get().getUserInputGnssSvIdSource
#define getUserInputGnssSvType GmccUtils::get().getUserInputGnssSvType
#define getUserInputGnssSvTypesMask GmccUtils::get().getUserInputGnssSvTypesMask
#define printGnssConfigBlacklistSvId GmccUtils::get().printGnssConfigBlacklistSvId
#define convertGnssSvIdMaskToList GmccUtils::get().convertGnssSvIdMaskToList
#define strUtilTokenize GmccUtils::get().strUtilTokenize


/* GnssMultiClientHidlILocHidlGnss static elements */
GnssMultiClientHidlILocHidlGnss* GnssMultiClientHidlILocHidlGnss::sInstance = nullptr;

/* GnssMultiClientHidlILocHidlGnss Public APIs */
GARDEN_RESULT GnssMultiClientHidlILocHidlGnss::menuTest()
{
    bool exit_loop = false;
    GARDEN_RESULT gardenResult = GARDEN_RESULT_INVALID;

    while(!exit_loop) {
        PRINTLN("\n\n"
                "1: Create HIDL client for ILocHidlGnss 3.0 \n"
                "1001: ILocHidlIzatConfig 1.0 -> init \n"
                "1002: ILocHidlIzatConfig 1.0 -> readConfig \n"
                "2001: ILocHidlIzatSubscription 1.0 -> init \n"
                "2002: ILocHidlIzatSubscription -> boolDataItemUpdate (Opt-in)\n"
                "99: Display this menu again \n"
                "0: <-- back\n");
        int choice = getUserInputInt("Enter choice: ");

        switch (choice) {
        case 1:
            gardenResult = createHidlClient();
            break;
        case 1001:
            gardenResult = ILocHidlIzatConfig_1_0_init();
            break;
        case 1002:
            gardenResult = ILocHidlIzatConfig_1_0_readConfig();
            break;
        case 2001:
            gardenResult = ILocHidlIzatSubscription_1_0_init();
            break;
        case 2002:
            {
                int result = getUserInputInt("Enable: 1, disable: 0");
                gardenResult = ILocHidlIzatSubscription_1_1_boolDataItemUpdate(result);
                break;
            }
        case 99:
            continue;
        case 0:
            gardenResult = GARDEN_RESULT_ABORT;
            exit_loop = true;
            break;
        default:
            PRINTERROR("Invalid command");
        }

        if (0 != choice) {
            PRINTLN("\nExecution Result: %d", gardenResult);
            getUserInputEnterToContinue();
        }
    }

    return gardenResult;
}

/* Callbacks registered with HIDL framework */
void LocHidlGnssDeathRecipient::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/) {
    PRINTERROR("ILocHidlGnss service died");
    GnssMultiClientHidlILocHidlGnss::get().mLocHidlGnssIface = nullptr;
    //get LocHidl service
    GnssMultiClientHidlILocHidlGnss::get().createHidlClient();
}

template <class T>
GARDEN_RESULT GnssMultiClientHidlILocHidlGnss::checkResultOk(T& Result,
    const std::string printMsg) {
    if (Result.isOk()) {
        PRINTLN("%s success.", printMsg.c_str());
        return GARDEN_RESULT_PASSED;
    } else {
        PRINTERROR("%s failed.", printMsg.c_str());
        return GARDEN_RESULT_FAILED;
    }
}

/* GnssMultiClientHidlILocHidlGnss TEST CASES */
GARDEN_RESULT GnssMultiClientHidlILocHidlGnss::createHidlClient()
{
    GARDEN_RESULT gardenResult = GARDEN_RESULT_FAILED;
    if (mLocHidlGnssIface) {
        PRINTLN("ILocHidlGnss instance already initalized!");
        return GARDEN_RESULT_PASSED;
    }
    // Get IGNSS service
    mLocHidlGnssIface = ILocHidlGnss::getService(GNSS_VENDOR_SERVICE_INSTANCE);
    if (mLocHidlGnssIface != nullptr) {
        PRINTLN("ILocHidlGnss::getService() success.");

        // Link to IGNSS service Death
        mLocHidlGnssDeathRecipient = new LocHidlGnssDeathRecipient();
        android::hardware::Return<bool> linked =
            mLocHidlGnssIface->linkToDeath(mLocHidlGnssDeathRecipient, 0);
        if (linked.isOk() && linked) {
            PRINTLN("mLocHidlGnssIface->linkToDeath() success.");
            // Get Extension : IGnssConfiguration 1.1
            auto izatConfig = mLocHidlGnssIface->getExtensionLocHidlIzatConfig();
            gardenResult = checkResultOk(izatConfig,
                               "mLocHidlGnssIface->getExtensionLocHidlIzatConfig");
            mLocHidlGnssExtensionIzatConfig = izatConfig;

            auto izatSubscribe = mLocHidlGnssIface->getExtensionLocHidlIzatSubscription_1_1();
            gardenResult = checkResultOk(izatSubscribe,
                               "mLocHidlGnssIface->getExtensionLocHidlIzatSubscription");
            mLocHidlIzatSubscription = izatSubscribe;
        } else {
            PRINTERROR("mLocHidlGnssIface->linkToDeath() failed, error: %s",
                    linked.description().c_str());
        }
    } else {
        PRINTERROR("ILocHidlGnss::getService() call failed.");
    }

    return gardenResult;
}

template <class T>
Return<bool> GnssMultiClientHidlILocHidlGnss::ILocHidlExtinit(T& ext) {
    return ext->init(mLocHidlGnssCb);
}

GARDEN_RESULT GnssMultiClientHidlILocHidlGnss::ILocHidlIzatConfig_1_0_init()
{
    GARDEN_RESULT gardenResult = GARDEN_RESULT_FAILED;

    if (mLocHidlGnssIface != nullptr) {
        if (mLocHidlGnssExtensionIzatConfig != nullptr) {
            auto result = ILocHidlExtinit(mLocHidlGnssExtensionIzatConfig);
            gardenResult = checkResultOk(result, "mLocHidlGnssExtensionIzatConfig->init");
        } else {
            PRINTERROR("mLocHidlGnssExtensionIzatConfig null");
        }
    } else {
        PRINTERROR("LOC HIDL client not created.");
    }

    return gardenResult;
}

GARDEN_RESULT GnssMultiClientHidlILocHidlGnss::ILocHidlIzatConfig_1_0_readConfig()
{
    GARDEN_RESULT gardenResult = GARDEN_RESULT_FAILED;

    if (mLocHidlGnssIface != nullptr) {
        if (mLocHidlGnssExtensionIzatConfig != nullptr) {
            auto result = mLocHidlGnssExtensionIzatConfig->readConfig();
            gardenResult = checkResultOk(result, "mLocHidlGnssExtensionIzatConfig->readConfig");
        } else {
            PRINTERROR("mLocHidlGnssExtensionIzatConfig null");
        }
    } else {
        PRINTERROR("LOC HIDL client not created.");
    }
    return gardenResult;
}

GARDEN_RESULT GnssMultiClientHidlILocHidlGnss::ILocHidlIzatSubscription_1_0_init() {
    GARDEN_RESULT gardenResult = GARDEN_RESULT_FAILED;
    if (mLocHidlGnssIface != nullptr) {
        if (mLocHidlIzatSubscription != nullptr) {
            auto result = ILocHidlExtinit(mLocHidlIzatSubscription);
            gardenResult = checkResultOk(result, "mLocHidlIzatSubscription->init");
        } else {
            PRINTERROR("mLocHidlIzatSubscription null");
        }
    } else {
        PRINTERROR("LOC HIDL client not created.");
    }
    return gardenResult;
}

GARDEN_RESULT GnssMultiClientHidlILocHidlGnss::ILocHidlIzatSubscription_1_1_boolDataItemUpdate
    (int result) {
    std::vector<ILocHidlIzatSubscription::BoolDataItem> dataItemArray;
    ILocHidlIzatSubscription::BoolDataItem dataItem;
    dataItem.id = ENH_DATA_ITEM_ID;
    dataItem.enabled = (result > 0) ? true: false;
    dataItemArray.push_back(dataItem);
    auto r = mLocHidlIzatSubscription->boolDataItemUpdate(dataItemArray);
    return checkResultOk(r, "mLocHidlIzatSubscription->boolDataItemUpdate");
}

} // namespace garden
