/* ControlGroup.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.mmca.vppp.db;

import android.os.Parcel;
import android.os.Parcelable;

import java.util.Collections;
import java.util.ArrayList;
import java.util.List;

/**
 * POJO to match the JSON schema for configuration ControlGroup.
 */
public class ControlGroup implements Parcelable {

    public static class Keys {
        public static final String KEY = "control-groups";
        public static final String ID = "id";
        public static final String DISPLAY_NAME = "display-name";
        public static final String CONTROL_FIELD_IDS = "control-field-ids";
    }

    private String mId;
    private String mDisplayName;
    private List<String> mControlFieldIds;

    private List<ControlField> mControlFields;

    public ControlGroup(String id, String displayName, List<String> controlFieldIds) {
        this.mId = id;
        this.mDisplayName = displayName;
        this.mControlFieldIds = controlFieldIds;

        this.mControlFields = Collections.emptyList();
    }

    public ControlGroup(ControlGroup controlGroup) {
        this.mId = controlGroup.mId;
        this.mDisplayName = controlGroup.mDisplayName;

        this.mControlFieldIds = new ArrayList<>();
        this.mControlFieldIds.addAll(controlGroup.mControlFieldIds);

        this.mControlFields = new ArrayList<>();
        for (ControlField controlField : controlGroup.mControlFields) {
            if (controlField != null) {
                this.mControlFields.add(new ControlField(controlField));
            }
        }
    }

    private ControlGroup(Parcel in) {
        this.mId = in.readString();
        this.mDisplayName = in.readString();
        this.mControlFieldIds = in.createStringArrayList();
        this.mControlFields = in.createTypedArrayList(ControlField.CREATOR);
    }

    public String getId() {
        return mId;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public List<String> getControlFieldIds() {
        return Collections.unmodifiableList(mControlFieldIds);
    }

    public List<ControlField> getControlFields() {
        return Collections.unmodifiableList(mControlFields);
    }

    /**
     * @param controlFields Should not be null.
     */
    void setControlFields(List<ControlField> controlFields) {
        this.mControlFields = controlFields;
    }

    public static final Parcelable.Creator<ControlGroup> CREATOR =
        new Parcelable.Creator<ControlGroup>() {
        @Override
        public ControlGroup createFromParcel(Parcel source) {
            return new ControlGroup(source);
        }

        @Override
        public ControlGroup[] newArray(int size) {
            return new ControlGroup[size];
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
        dest.writeStringList(this.mControlFieldIds);
        dest.writeTypedList(this.mControlFields);
    }

}
