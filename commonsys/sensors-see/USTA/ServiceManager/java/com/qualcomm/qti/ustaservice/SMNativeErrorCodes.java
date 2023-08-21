/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.os.Parcel;
import android.os.Parcelable;

public enum SMNativeErrorCodes implements Parcelable {
    USTA_MEMORY_ERROR,
    USTA_PROTOFILES_NOT_FOUND,
    USTA_REQUIRED_FIELDS_MISSING,
    USTA_NO_ERROR;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeInt(this.ordinal());
    }

    public static final Parcelable.Creator<SMNativeErrorCodes> CREATOR =
            new Parcelable.Creator<SMNativeErrorCodes> () {
        @Override
        public SMNativeErrorCodes createFromParcel(Parcel parcel) {
            return values()[parcel.readInt()];
        }

        @Override
        public SMNativeErrorCodes[] newArray(int size) {
            return new SMNativeErrorCodes[size];
        }
    };
}
