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

public class USTASensorInfo implements Parcelable {
    public List<String> clientProcessorTypes = new ArrayList<String>();
    public List<String> wakeupDeliveryTypes = new ArrayList<String>();
    public List<SMSensorInfo> SensorsInfo = new ArrayList<SMSensorInfo>();

    public USTASensorInfo(Parcel in ) {
         in.readStringList(clientProcessorTypes);
         in.readStringList(wakeupDeliveryTypes);
         in.readTypedList(SensorsInfo, SMSensorInfo.CREATOR);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int flags) {
        parcel.writeStringList(clientProcessorTypes);
        parcel.writeStringList(wakeupDeliveryTypes);
        parcel.writeTypedList(SensorsInfo);
    }
    public static final Parcelable.Creator<USTASensorInfo> CREATOR =
            new Parcelable.Creator<USTASensorInfo>() {
        @Override
        public USTASensorInfo createFromParcel(Parcel inParcel) {
            return new USTASensorInfo(inParcel);
        }

        @Override
        public USTASensorInfo[] newArray(int size) {
            return new USTASensorInfo[size];
        }
    };

    public USTASensorInfo(List<String> inClientProcessorTypes, List<String> inWakeupDeliveryTypes, List<SMSensorInfo> inSensorsInfo) {
        this.clientProcessorTypes = inClientProcessorTypes;
        this.wakeupDeliveryTypes = inWakeupDeliveryTypes;
        for(int sensorIndex = 0; sensorIndex < inSensorsInfo.size(); sensorIndex++ ) {
            SMSensorInfo it = new SMSensorInfo(inSensorsInfo.get(sensorIndex).dataType,
                    inSensorsInfo.get(sensorIndex).vendor,
                    inSensorsInfo.get(sensorIndex).suidLow,
                    inSensorsInfo.get(sensorIndex).suidHigh,
                    inSensorsInfo.get(sensorIndex).attributesInfo,
                    inSensorsInfo.get(sensorIndex).requestMsgs);
            this.SensorsInfo.add(it);
        }
    }
}
