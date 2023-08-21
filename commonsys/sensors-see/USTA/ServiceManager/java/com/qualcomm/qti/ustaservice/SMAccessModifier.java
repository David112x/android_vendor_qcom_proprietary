/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;


import android.os.Parcel;
import android.os.Parcelable;

public enum SMAccessModifier implements Parcelable {
    REQUIRED,
    OPTIONAL,
    REPEATED;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeInt(this.ordinal());
    }
    public static final Parcelable.Creator<SMAccessModifier> CREATOR = new Parcelable.Creator<SMAccessModifier>() {
        @Override
        public SMAccessModifier createFromParcel(Parcel parcel) {
            return values()[parcel.readInt()];
        }

        @Override
        public SMAccessModifier[] newArray(int size) {
            return new SMAccessModifier[size];
        }
    };
}
