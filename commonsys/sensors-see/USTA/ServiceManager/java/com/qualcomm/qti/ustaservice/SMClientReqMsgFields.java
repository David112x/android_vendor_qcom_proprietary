/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import android.os.Parcel;
import android.os.Parcelable;

public class SMClientReqMsgFields implements Parcelable {
    public String batchPeriod;
    public String flushPeriod;
    public String clientType;
    public String wakeupType;
    public String flushOnly;
    public String maxBatch;
    public String isPassive;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeString(batchPeriod);
        parcel.writeString(flushPeriod);
        parcel.writeString(clientType);
        parcel.writeString(wakeupType);
        parcel.writeString(flushOnly);
        parcel.writeString(maxBatch);
        parcel.writeString(isPassive);
    }

    public static final Parcelable.Creator<SMClientReqMsgFields> CREATOR = new Parcelable.Creator<SMClientReqMsgFields>() {
        @Override
        public SMClientReqMsgFields createFromParcel(Parcel parcel) {
            return new SMClientReqMsgFields(parcel);
        }

        @Override
        public SMClientReqMsgFields[] newArray(int size) {
            return new SMClientReqMsgFields[size];
        }
    };
    public SMClientReqMsgFields(Parcel in) {
        batchPeriod = in.readString();
        flushPeriod = in.readString();
        clientType = in.readString();
        wakeupType = in.readString();
        flushOnly = in.readString();
        maxBatch = in.readString();
        isPassive = in.readString();
    }

    public SMClientReqMsgFields(String batchPeriod, String flushPeriod,
                                String clientType, String wakeupType,
                                String flushOnly, String maxBatch, String isPassive) {
        this.batchPeriod = batchPeriod;
        this.clientType = clientType;
        this.flushOnly = flushOnly;
        this.flushPeriod = flushPeriod;
        this.wakeupType = wakeupType;
        this.maxBatch = maxBatch;
        this.isPassive = isPassive;
    }
}
