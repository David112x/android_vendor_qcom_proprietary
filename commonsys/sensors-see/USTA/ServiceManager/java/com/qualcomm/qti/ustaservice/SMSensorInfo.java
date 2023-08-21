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

public class SMSensorInfo implements Parcelable {

    public String dataType;
    public String vendor;
    public String suidLow;
    public String suidHigh;
    public String attributesInfo;
    public List<SMReqMsgPayload> requestMsgs = new ArrayList<SMReqMsgPayload>();

    public SMSensorInfo(Parcel in) {
        dataType = in.readString();
        vendor = in.readString();
        suidLow = in.readString();
        suidHigh = in.readString();
        attributesInfo = in.readString();
        in.readTypedList(requestMsgs, SMReqMsgPayload.CREATOR);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int flags) {
        parcel.writeString(dataType);
        parcel.writeString(vendor);
        parcel.writeString(suidLow);
        parcel.writeString(suidHigh);
        parcel.writeString(attributesInfo);
        parcel.writeTypedList(requestMsgs);
    }

    public static final Parcelable.Creator<SMSensorInfo> CREATOR = new Parcelable.Creator<SMSensorInfo>() {
        @Override
        public SMSensorInfo createFromParcel(Parcel parcel) {
            return new SMSensorInfo(parcel);
        }

        @Override
        public SMSensorInfo[] newArray(int size) {
            return new SMSensorInfo[size];
        }
    };
    public SMSensorInfo(String inDataType, String inVendor, String inSuidLow, String inSuidHigh,
                        String inAttributesInfo, List<SMReqMsgPayload> inRequestMsgs) {
        this.dataType = inDataType;
        this.vendor = inVendor;
        this.suidLow = inSuidLow;
        this.suidHigh = inSuidHigh;
        this.attributesInfo = inAttributesInfo;
        this.requestMsgs = inRequestMsgs;
    }

}
