/* Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package org.codeaurora.ims;

import android.telephony.ims.feature.CapabilityChangeRequest.CapabilityPair;
import android.telephony.ims.feature.MmTelFeature.MmTelCapabilities;
import android.telephony.ims.stub.ImsRegistrationImplBase;

import java.util.concurrent.CopyOnWriteArrayList;

/* This class keeps track of enabled MmTelFeature.MmTelCapabilities.MmTelCapability
 * It stores the capability with the radio tech within a CapabilityPair
 */
public class CapabilityTracker {

    private CopyOnWriteArrayList<CapabilityPair> mCapabilityContainer;
    // CallComposer capability is not supported with MmTelCapability
    private boolean mIsCallComposerSupported = false;

    public CapabilityTracker() {
        mCapabilityContainer = new CopyOnWriteArrayList<CapabilityPair>();
    }

    // Adds CapabilityPair to the arrayList if not already present
    public void addCapability(@MmTelCapabilities.MmTelCapability int capability,
            @ImsRegistrationImplBase.ImsRegistrationTech int radioTech) {
        CapabilityPair cp = new CapabilityPair(capability, radioTech);
        if (!mCapabilityContainer.contains(cp)) {
            mCapabilityContainer.add(cp);
        }
    }

    // Removes CapabilityPair from the arrayList if present
    public void removeCapability(@MmTelCapabilities.MmTelCapability int capability,
            @ImsRegistrationImplBase.ImsRegistrationTech int radioTech) {
        CapabilityPair cp = new CapabilityPair(capability, radioTech);
        if (mCapabilityContainer.contains(cp)) {
            mCapabilityContainer.remove(cp);
        }
    }

    // Clears arrayList of CapabilityPairs
    public void clear() {
        mCapabilityContainer.clear();
        mIsCallComposerSupported = false;
    }

    // Checks if CapabilityPair with capability type VIDEO
    // is present
    public boolean isVideoSupported() {
        return isSupported(MmTelCapabilities.CAPABILITY_TYPE_VIDEO);
    }

    // Checks if CapabilityPair with capability type VOICE
    // is present
    public boolean isVoiceSupported() {
        return isSupported(MmTelCapabilities.CAPABILITY_TYPE_VOICE);
    }

    // Checks if CapabilityPair with capability type VIDEO
    // and registration tech IWLAN is present
    public boolean isVideoSupportedOverWifi() {
        return isSupportedOnRadioTech(MmTelCapabilities.CAPABILITY_TYPE_VIDEO,
                ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN);
    }

    // Checks if CapabilityPair with capability type VOICE
    // and registration tech IWLAN is present
    public boolean isVoiceSupportedOverWifi() {
        return isSupportedOnRadioTech(MmTelCapabilities.CAPABILITY_TYPE_VOICE,
                ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN);
    }

    // Returns the list of enabled capabilities
    public CopyOnWriteArrayList<CapabilityPair> getEnabledCapabilities() {
        return mCapabilityContainer;
    }

    /**
     * Utility method that checks if the arrayList contains a CapabilityPair
     * with the requested capability
     */
    public boolean isSupported(@MmTelCapabilities.MmTelCapability int capability) {
        for (CapabilityPair cp: mCapabilityContainer) {
            if (cp.getCapability() == capability) {
                return true;
            }
        }
        return false;
    }

    /**
     * Utility method that checks if the arrayList contains a CapabilityPair
     * with the requested capability and radio tech
     */
    public boolean isSupportedOnRadioTech(@MmTelCapabilities.MmTelCapability int capability,
            @ImsRegistrationImplBase.ImsRegistrationTech int radioTech) {
        for (CapabilityPair cp : mCapabilityContainer) {
            if (cp.getCapability() == capability && cp.getRadioTech() == radioTech) {
                return true;
            }
        }
        return false;
    }

    /**
     * CallComposer capability is not supported within MmTelCapability
     * and frameworks is not aware of this new capability
     * Adding utility methods to set and retrieve the status
     */
    public void setIsCallComposerSupported(boolean isSupported) {
        mIsCallComposerSupported = isSupported;
    }

    public boolean isCallComposerSupported() {
        return mIsCallComposerSupported;
    }
}
