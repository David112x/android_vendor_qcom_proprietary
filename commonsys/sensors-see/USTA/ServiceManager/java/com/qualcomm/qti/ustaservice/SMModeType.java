/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.os.Parcel;
import android.os.Parcelable;

public enum SMModeType implements Parcelable{
    USTA_MODE_TYPE_UI,
    USTA_MODE_TYPE_COMMAND_LINE,
    USTA_MODE_TYPE_COUNT;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeInt(this.ordinal());
    }
    public static final Parcelable.Creator<SMModeType> CREATOR = new Parcelable.Creator<SMModeType>() {
        @Override
        public SMModeType[] newArray(int size) {
            return new SMModeType[size];
        }

        @Override
        public SMModeType createFromParcel(Parcel parcel) {
            return values()[parcel.readInt()];
        }
    };
 }
