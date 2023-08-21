/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.os.Parcel;
import android.os.Parcelable;

import java.util.ArrayList;
import java.util.List;

public class SMNestedMsgPayload implements Parcelable {
    public String nestedMsgName;
    public List<SMSensorPayloadField> fields = new ArrayList<SMSensorPayloadField>();

    public SMNestedMsgPayload(Parcel in) {
        nestedMsgName = in.readString();
        in.readTypedList(fields, SMSensorPayloadField.CREATOR);
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeString(nestedMsgName);
        parcel.writeTypedList(fields);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Parcelable.Creator<SMNestedMsgPayload> CREATOR = new Parcelable.Creator<SMNestedMsgPayload>() {
        @Override
        public SMNestedMsgPayload createFromParcel(Parcel parcel) {
            return new SMNestedMsgPayload(parcel);
        }

        @Override
        public SMNestedMsgPayload[] newArray(int size) {
            return new SMNestedMsgPayload[size];
        }
    };

    public SMNestedMsgPayload(String inNestedMsgName, List<SMSensorPayloadField> inFields) {
        this.nestedMsgName = inNestedMsgName;
        this.fields = inFields;
    }
}
