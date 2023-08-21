/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.os.Parcel;
import android.os.Parcelable;

public class SMEumInfo implements Parcelable {
    public String name;
    public int value;

    public SMEumInfo(Parcel in) {
        name = in.readString();
        value = in.readInt();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeString(name);
        parcel.writeInt(value);
    }

    public static final Parcelable.Creator<SMEumInfo> CREATOR = new Parcelable.Creator<SMEumInfo>() {
        @Override
        public SMEumInfo createFromParcel(Parcel parcel) {
            return new SMEumInfo(parcel);
        }

        @Override
        public SMEumInfo[] newArray(int size) {
            return new SMEumInfo[size];
        }
    };

    public SMEumInfo(String inName, int inValue) {
        this.name = inName;
        this.value = inValue;
    }

}
