/* ChipConfig.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.db;

import android.os.Parcel;
import android.os.Parcelable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * POJO to match the JSON schema for configuration ChipConfig.
 */
public class ChipConfig implements Parcelable {

    public static class Keys {
        public static final String KEY = "chip-definitions";
        public static final String TARGET = "target";
        public static final String CONFIGS = "configs";
        public static final String OS_VERSION = "os-version";
        public static final String PRESET_IDS = "preset-ids";
    }

    private String mTarget;
    private List<Config> mConfigs;

    public ChipConfig(String target, List<Config> configs) {
        this.mTarget = target;

        if (configs == null) {
            mConfigs = Collections.emptyList();
        } else {
            this.mConfigs = configs;
        }
    }

    public ChipConfig(ChipConfig chipConfig) {
        this.mTarget = chipConfig.mTarget;

        this.mConfigs = new ArrayList<>();
        for (Config config : chipConfig.mConfigs) {
            if (config != null) {
                this.mConfigs.add(new Config(config));
            }
        }
    }

    private ChipConfig(Parcel in) {
        this.mTarget = in.readString();
        this.mConfigs = in.createTypedArrayList(Config.CREATOR);
    }

    public String getTarget() {
        return mTarget;
    }

    public List<Config> getConfigs() {
        return Collections.unmodifiableList(mConfigs);
    }

    /**
     * @param configs Should not be null.
     */
    void setConfigs(List<Config> configs) {
        this.mConfigs = configs;
    }

    public static final Parcelable.Creator<ChipConfig> CREATOR =
        new Parcelable.Creator<ChipConfig>() {
        @Override
        public ChipConfig createFromParcel(Parcel source) {
            return new ChipConfig(source);
        }

        @Override
        public ChipConfig[] newArray(int size) {
            return new ChipConfig[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.mTarget);
        dest.writeTypedList(this.mConfigs);
    }

    public static class Config implements Parcelable {

        private int mOSVersion;
        private List<String> mPresetIds;

        private List<Preset> mPresets;

        /**
         * @param osVersion
         * @param presetIds Should not be null.
         */
        public Config(int osVersion, List<String> presetIds) {
            this.mOSVersion = osVersion;
            this.mPresetIds = presetIds;

            mPresets = Collections.emptyList();
        }

        public Config(Config config) {
            this.mOSVersion = config.mOSVersion;

            this.mPresetIds = new ArrayList<>();
            if (config.mPresetIds != null) {
                this.mPresetIds.addAll(config.mPresetIds);
            }

            this.mPresets = new ArrayList<>();
            for (Preset preset : config.mPresets) {
                if (preset != null) {
                    this.mPresets.add(new Preset(preset));
                }
            }
        }

        private Config(Parcel in) {
            this.mOSVersion = in.readInt();
            this.mPresetIds = in.createStringArrayList();
            this.mPresets = in.createTypedArrayList(Preset.CREATOR);
        }

        public int getOSVersion() {
            return mOSVersion;
        }

        public List<String> getPresetIds() {
            return Collections.unmodifiableList(mPresetIds);
        }

        /**
         * @return Deep copy of the Presets
         */
        public List<Preset> getPresets() {
            List<Preset> copies = new ArrayList<>();
            mPresets.forEach(preset -> copies.add(new Preset(preset)));
            return copies;
        }

        /**
         * @param presets  Should not be null.
         */
        void setPresets(List<Preset> presets) {
            this.mPresets = presets;
        }

        public static final Creator<Config> CREATOR = new Creator<Config>() {
            @Override
            public Config createFromParcel(Parcel source) {
                return new Config(source);
            }

            @Override
            public Config[] newArray(int size) {
                return new Config[size];
            }
        };

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(this.mOSVersion);
            dest.writeStringList(this.mPresetIds);
            dest.writeTypedList(this.mPresets);
        }

    }
}
