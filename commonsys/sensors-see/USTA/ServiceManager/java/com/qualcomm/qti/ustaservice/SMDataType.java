/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.os.Parcel;
import android.os.Parcelable;

public enum SMDataType implements Parcelable {
    STRING,
    SIGNED_INTEGER32,
    SIGNED_INTEGER64,
    UNSIGNED_INTEGER32,
    UNSIGNED_INTEGER64,
    FLOAT,
    BOOLEAN,
    ENUM,
    USER_DEFINED;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeInt(this.ordinal());
    }
    public static final Parcelable.Creator<SMDataType> CREATOR = new Parcelable.Creator<SMDataType>(){
        @Override
        public SMDataType createFromParcel(Parcel parcel) {
            return values()[parcel.readInt()];
        }

        @Override
        public SMDataType[] newArray(int size) {
            return new SMDataType[size];
        }
    };

}
