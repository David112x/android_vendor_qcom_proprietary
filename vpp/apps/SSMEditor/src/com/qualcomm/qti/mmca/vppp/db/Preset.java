/* Preset.java
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
 * POJO to match the JSON schema for configuration Preset.
 */
public class Preset implements Parcelable {

    public static class Keys {
        public static final String KEY = "presets";
        public static final String ID = "id";
        public static final String DISPLAY_NAME = "display-name";
        public static final String CONTROLS = "controls";
        public static final String CONTROL_GROUP_ID = "control-group-id";
        public static final String FIELDS = "fields";
    }

    private String mId;
    private String mDisplayName;
    private List<OverrideControl> mOverrideControls;

    private List<ControlGroup> mControlGroups;

    public Preset(String id, String displayName, List<OverrideControl> overrideControls) {
        this.mId = id;
        this.mDisplayName = displayName;
        this.mOverrideControls = overrideControls;

        mControlGroups = Collections.emptyList();
    }

    public Preset(Preset preset) {
        this.mId = preset.mId;
        this.mDisplayName = preset.mDisplayName;

        this.mOverrideControls = new ArrayList<>();
        for (OverrideControl control : preset.mOverrideControls) {
            if (control != null) {
                this.mOverrideControls.add(new OverrideControl(control));
            }
        }

        this.mControlGroups = new ArrayList<>();
        for (ControlGroup controlGroup : preset.mControlGroups) {
            if (controlGroup != null) {
                this.mControlGroups.add(new ControlGroup(controlGroup));
            }
        }
    }

    private Preset(Parcel in) {
        this.mId = in.readString();
        this.mDisplayName = in.readString();
        this.mOverrideControls = in.createTypedArrayList(OverrideControl.CREATOR);
        this.mControlGroups = in.createTypedArrayList(ControlGroup.CREATOR);
    }

    public String getId() {
        return mId;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public List<OverrideControl> getOverrideControls() {
        return Collections.unmodifiableList(mOverrideControls);
    }

    public List<ControlGroup> getControlGroups() {
        return Collections.unmodifiableList(mControlGroups);
    }

    /**
     * @param controlGroups  Should not be null.
     */
    void setControlGroups(List<ControlGroup> controlGroups) {
        this.mControlGroups = controlGroups;
    }

    public static final Parcelable.Creator<Preset> CREATOR = new Parcelable.Creator<Preset>() {
        @Override
        public Preset createFromParcel(Parcel source) {
            return new Preset(source);
        }

        @Override
        public Preset[] newArray(int size) {
            return new Preset[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.mId);
        dest.writeString(this.mDisplayName);
        dest.writeTypedList(this.mOverrideControls);
        dest.writeTypedList(this.mControlGroups);
    }

    public static class OverrideControl implements Parcelable {

        private String mControlGroupId;
        private List<FieldOverride> mFieldsToOverride;

        public OverrideControl(String controlGroupId) {
            this.mControlGroupId = controlGroupId;
            mFieldsToOverride = Collections.emptyList();
        }

        /**
         * @param controlGroupId
         * @param fieldsToOverride  Should not be null.
         */
        public OverrideControl(String controlGroupId, List<FieldOverride> fieldsToOverride) {
            this.mControlGroupId = controlGroupId;
            this.mFieldsToOverride = fieldsToOverride;
        }

        public OverrideControl(OverrideControl control) {
            this.mControlGroupId = control.mControlGroupId;
            this.mFieldsToOverride = new ArrayList<>();

            for (FieldOverride fieldOverride : control.mFieldsToOverride) {
                if (fieldOverride != null) {
                    this.mFieldsToOverride.add(new FieldOverride(fieldOverride));
                }
            }
        }

        public String getControlGroupId() {
            return mControlGroupId;
        }

        public List<FieldOverride> getFieldsToOverride() {
            return Collections.unmodifiableList(mFieldsToOverride);
        }

        public static final Parcelable.Creator<OverrideControl> CREATOR =
            new Parcelable.Creator<OverrideControl>() {
            @Override
            public OverrideControl createFromParcel(Parcel source) {
                return new OverrideControl(source);
            }

            @Override
            public OverrideControl[] newArray(int size) {
                return new OverrideControl[size];
            }
        };

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(this.mControlGroupId);
        }

        protected OverrideControl(Parcel in) {
            this.mControlGroupId = in.readString();
        }

        // TODO see if this needs to be Parcelable
        public static class FieldOverride {

            private String fieldId;
            private Object value;

            public FieldOverride(String fieldId, Object value) {
                this.fieldId = fieldId;
                this.value = value;
            }

            public FieldOverride(FieldOverride fieldOverride) {
                this.fieldId = fieldOverride.fieldId;
                this.value = fieldOverride.value;
            }

            public String getFieldId() {
                return fieldId;
            }

            /**
             * Retrieve the value. User should downcast based on the {@link VendorExtType} of the
             * ControlField
             * @return
             */
            public Object getValue() {
                return value;
            }

        }

    }

}
