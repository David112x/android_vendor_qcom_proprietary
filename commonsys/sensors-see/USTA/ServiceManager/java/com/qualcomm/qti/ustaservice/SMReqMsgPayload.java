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

public class SMReqMsgPayload implements Parcelable {
    public String msgName;
    public String msgID;
    public List<SMSensorPayloadField> fieldInfoVar = new ArrayList<SMSensorPayloadField>();

    public SMReqMsgPayload(Parcel in) {
        msgName = in.readString();
        msgID = in.readString();
        in.readTypedList(fieldInfoVar, SMSensorPayloadField.CREATOR);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeString(msgName);
        parcel.writeString(msgID);
        parcel.writeTypedList(fieldInfoVar);
    }

    public static final Parcelable.Creator<SMReqMsgPayload> CREATOR = new Parcelable.Creator<SMReqMsgPayload>() {
        @Override
        public SMReqMsgPayload createFromParcel(Parcel parcel) {
            return new SMReqMsgPayload(parcel);
        }

        @Override
        public SMReqMsgPayload[] newArray(int size) {
            return new SMReqMsgPayload[size];
        }
    };

    public SMReqMsgPayload(String inMsgName, String inMsgID , List<SMSensorPayloadField> inFieldInfo) {
        this.msgName = inMsgName;
        this.msgID = inMsgID;
        this.fieldInfoVar = inFieldInfo;
    }
}
