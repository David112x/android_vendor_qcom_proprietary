/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file VPPPActivityViewModel.java
 */

package com.qualcomm.qti.mmca.vppp;

import android.media.MediaFormat;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;

import com.qualcomm.qti.ssmeditor.QtiApplication;
import com.qualcomm.qti.mmca.decoder.MediaCodecParams;
import com.qualcomm.qti.mmca.vppp.db.ChipConfig;
import com.qualcomm.qti.mmca.vppp.db.ControlField;
import com.qualcomm.qti.mmca.vppp.db.ControlGroup;
import com.qualcomm.qti.mmca.vppp.db.Preset;
import com.qualcomm.qti.mmca.vppp.ui.ExtensionHolder;

import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.List;

import org.json.JSONException;

/**
 * ViewModel for VPPPActivity.
 */
public class VPPPActivityViewModel implements Parcelable {

    private QtiApplication mApplication;
    private String mTarget;
    private int mSdkVer;

    private ChipConfig.Config mConfig;
    private int mSelectedPresetIndex;
    private Preset mSelectedPreset;
    private BottomSheetState mBottomSheetState;

    private Uri mVideoUri;
    private long mCurrentVideoTime;
    private boolean mIsPlaying;

    /**
     * @param application Application context. Should not be null.
     * @param target Board name (which should match a configuration target).
     * @param sdkVer
     */
    public VPPPActivityViewModel(QtiApplication application,
                                 String target, int sdkVer, Uri videoUri)
                                 throws JSONException, IllegalArgumentException {
        mApplication = application;
        mTarget = target;
        mSdkVer = sdkVer;
        mVideoUri = videoUri;

        mCurrentVideoTime = 0;
        mIsPlaying = true;

        mSelectedPresetIndex = -1;
        mBottomSheetState = BottomSheetState.COLLAPSED;
        fetchConfig();
    }

    /**
     * Recreates the ViewModel with a Context which allows it to reload
     * the ConfigRepo. This should be called after loading an instance of
     * this class from a Parcel.
     *
     * @param application Application context. Should not be null.
     * @throws JSONException Config not found for the device.
     */
    public void reload(QtiApplication application) throws JSONException,
           IllegalArgumentException {
        this.mApplication = application;

        fetchConfig();
    }

    private void fetchConfig() throws JSONException, IllegalArgumentException {
        mConfig = mApplication.getConfigRepo().getChipConfig(mTarget, mSdkVer);

        if (mConfig == null) {
            throw new JSONException("No configurations found on device.");
        } else if (mConfig.getPresets().size() == 0) {
            throw new IllegalArgumentException("The chip configuration which matches this device"
                    + " does not have any presets");
        }
    }

    public ChipConfig.Config getConfig() {
        return mConfig;
    }

    public int getSelectedPresetIndex() {
        return mSelectedPresetIndex;
    }

    public void setSelectedPresetIndex(int index) {
        if (index != mSelectedPresetIndex) {
            mSelectedPresetIndex = index;
            mSelectedPreset = mConfig.getPresets().get(mSelectedPresetIndex);
        }
    }

    public Preset getSelectedPreset() {
        return mSelectedPreset;
    }

    public boolean isPresetSelected() {
        return mSelectedPresetIndex != -1;
    }

    public Uri getVideoUri() {
        return mVideoUri;
    }

    public void setCurrentVideoTime(long time) {
        if (time >= 0) {
            mCurrentVideoTime = time;
        }
    }

    public long getCurrentVideoTime() {
        return mCurrentVideoTime;
    }

    public void setIsPlaying(boolean isPlaying) {
        mIsPlaying = isPlaying;
    }

    public boolean getIsPlaying() {
        return mIsPlaying;
    }

    public void setBottomSheetState(BottomSheetState state) {
        mBottomSheetState = state;
    }

    public BottomSheetState getBottomSheetState() {
        return mBottomSheetState;
    }

    /**
     * Get a list of all the Preset names available for the config. Used for displaying
     * in the preset dialog picker.
     * @return List of the names of all Presets for the Config.
     */
    public String[] getPresetDisplayNames() {
        List<Preset> presetList = mConfig.getPresets();
        String[] presets = new String[presetList.size()];

        for (int i = 0; i < presets.length; i++) {
            presets[i] = presetList.get(i).getDisplayName();
        }

        return presets;
    }

    /**
     * Gets the current value of all the ControlFields in the selected Preset and
     * combines them into a single MediaCodecParams object which can then be passed
     * to the VPPPDecoder.
     */
    public MediaCodecParams getSelectedPresetAsParams() {
        Bundle accumulator = new Bundle();

        if (mSelectedPreset != null) {
            // Copy all the ControlField values in the accumulator
            List<ControlGroup> groups = mSelectedPreset.getControlGroups();
            for (ControlGroup group : groups) {
                List<ControlField> fields = group.getControlFields();
                for (ControlField field : fields) {
                    accumulator = controlFieldToBundle(field, accumulator);
                }
            }
        }

        return bundleToParams(accumulator);
    }

    /**
     * Maps the vendor extension and current value from the ControlField into a
     * the Bundle.
     */
    private Bundle controlFieldToBundle(ControlField from, Bundle to) {
        return new ExtensionHolder(from).pack(to);
    }

    /**
     * Convert the key-value mappings from the bundle into a new MediaCodecParams
     * instance.
     */
    private MediaCodecParams bundleToParams(final Bundle from) {
        return new MediaCodecParams() {
            @Override
            public MediaFormat pack(MediaFormat mediaFormat) {
                Iterator<String> iterator = from.keySet().iterator();

                while (iterator.hasNext()) {
                    String key = iterator.next();
                    Object value = from.get(key);

                    if (value != null) {
                        if (value instanceof Integer) {
                            mediaFormat.setInteger(key, (Integer) value);
                        } else if (value instanceof Long) {
                            mediaFormat.setLong(key, (Long) value);
                        } else if (value instanceof Float) {
                            mediaFormat.setFloat(key, (Float) value);
                        } else if (value instanceof String) {
                            mediaFormat.setString(key, (String) value);
                        } else if (value instanceof ByteBuffer) {
                            mediaFormat.setByteBuffer(key, (ByteBuffer) value);
                        }

                        // No other types are supported by MediaFormat so ignore
                    }
                }

                return mediaFormat;
            }

            @Override
            public Bundle pack(Bundle bundle) {
                bundle.putAll(from);
                return bundle;
            }
        };
    }

    public static final Parcelable.Creator<VPPPActivityViewModel> CREATOR =
        new Parcelable.Creator<VPPPActivityViewModel>() {
            @Override
            public VPPPActivityViewModel createFromParcel(Parcel source) {
                return new VPPPActivityViewModel(source);
            }

            @Override
            public VPPPActivityViewModel[] newArray(int size) {
                return new VPPPActivityViewModel[size];
            }
        };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.mTarget);
        dest.writeInt(this.mSdkVer);
        dest.writeInt(this.mSelectedPresetIndex);
        dest.writeParcelable(mSelectedPreset, 0);
        dest.writeLong(this.mCurrentVideoTime);
        dest.writeBoolean(this.mIsPlaying);
        dest.writeString(mBottomSheetState.name());
        dest.writeParcelable(this.mVideoUri, 0);
    }

    private VPPPActivityViewModel(Parcel in) {
        this.mTarget = in.readString();
        this.mSdkVer = in.readInt();
        this.mSelectedPresetIndex = in.readInt();
        this.mSelectedPreset = in.readParcelable(Preset.class.getClassLoader());
        this.mCurrentVideoTime = in.readLong();
        this.mIsPlaying = in.readBoolean();
        this.mBottomSheetState = BottomSheetState.valueOf(in.readString());
        this.mVideoUri = in.readParcelable(Uri.class.getClassLoader());
    }

}

