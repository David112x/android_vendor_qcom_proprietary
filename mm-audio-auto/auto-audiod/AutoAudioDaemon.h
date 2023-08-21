/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * Copyright (c) 2012-2014, 2019 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <vector>
#include <binder/IBinder.h>

#include <utils/threads.h>
#include <utils/String8.h>

#ifdef VHAL_EXT
#include "AutoPower.h"
#endif

#ifdef AHAL_EXT
#include <android/hardware/audio/5.0/IDevice.h>
#include <android/hardware/audio/5.0/IDevicesFactory.h>
#include <android/hardware/audio/5.0/types.h>

using ::android::hardware::audio::V5_0::IDevice;
using ::android::hardware::audio::V5_0::IDevicesFactory;
using ::android::hardware::audio::V5_0::Result;
using ::android::hardware::audio::V5_0::ParameterValue;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_death_recipient;
#endif

using ::android::sp;

namespace android {

enum notify_status {
    SND_CARD_ONLINE,
    SND_CARD_OFFLINE,
    POWER_D0, /* full On */
    POWER_D1, /* partial On */
    POWER_D2, /* partial On */
    POWER_D3hot, /* Off, with power */
    POWER_D3cold, /* Off, without power */
};

enum notify_status_type {
    SND_CARD_STATE,
    POWER_STATE
};

struct snd_card_fd {
    int snd_card;
    int state;
    int power;
};

class AutoAudioDaemon : public IBinder::DeathRecipient
{
    /*Overrides*/
    virtual void binderDied(const wp<IBinder> &the_late_who);

    bool getSndCardFDs(std::vector<snd_card_fd> &sndcardFds);
    void putSndCardFDs(std::vector<snd_card_fd> &sndcardFds);

#ifdef AHAL_EXT
    void notifyAudioDevice(int snd_card,
                           notify_status status,
                           notify_status_type type);
    void handleAudioHalDeath();

    class AudioHalDeathRecipient : public hidl_death_recipient
    {
        public:
            AudioHalDeathRecipient(const sp<AutoAudioDaemon> autoaudiodaemon) :
                    mAutoAudioDaemon(autoaudiodaemon) {}
            virtual void serviceDied(uint64_t cookie,
                    const wp<hidl::base::V1_0::IBase>& who);
        private:
            sp<AutoAudioDaemon> mAutoAudioDaemon;
    };
#endif

public:
    AutoAudioDaemon();
    virtual ~AutoAudioDaemon();

    bool threadLoop();

private:
    std::vector<snd_card_fd> mSndCardFd;
    std::vector<pthread_t> mThread;

#ifdef AHAL_EXT
    status_t parametersFromStr(const String8& key,
                                const String8& value,
                                hidl_vec<ParameterValue> *hidlParams);
    status_t getDeviceInstance();

    sp<IDevicesFactory> mDevicesFactory;
    sp<IDevice> mDevice;
    sp<AudioHalDeathRecipient> mAudioHalDeathRecipient;
#endif

#ifdef VHAL_EXT
    sp<AutoPower> mAutoPower;
#endif
};

}
