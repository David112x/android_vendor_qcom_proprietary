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

public class SMSensorPayloadField implements Parcelable {
    public String fieldName;
    public SMDataType SMDataType ;
    public SMAccessModifier SMAccessModifier;
    public boolean isEnumerated;
    public List<String> enumValues = new ArrayList<String>();
    public boolean hasDefaultValue;
    public String defaultValue;
    public List<String> value = new ArrayList<String>();
    public boolean isUserDefined;
    public List<SMNestedMsgPayload> nestedMsgs = new ArrayList<SMNestedMsgPayload>();
    public List<String> valuesForNative = new ArrayList<String>();

    public SMSensorPayloadField(Parcel in) {
        fieldName = in.readString();
        SMDataType = SMDataType.values()[in.readInt()];
        SMAccessModifier = SMAccessModifier.values()[in.readInt()];
        isEnumerated = in.readByte() != 0;
        in.readStringList(enumValues);
        hasDefaultValue = in.readByte() != 0;
        defaultValue = in.readString();
        in.readStringList(value);
        isUserDefined = in.readByte() != 0;
        in.readTypedList(nestedMsgs, SMNestedMsgPayload.CREATOR);
        in.readStringList(valuesForNative);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        parcel.writeString(fieldName);
        parcel.writeInt(SMDataType.ordinal());
        parcel.writeInt(SMAccessModifier.ordinal());
        parcel.writeByte((byte) (isEnumerated ? 1 : 0));
        parcel.writeStringList(enumValues);
        parcel.writeByte((byte) (hasDefaultValue ? 1 : 0));
        parcel.writeString(defaultValue);
        parcel.writeStringList(value);
        parcel.writeByte((byte)(isUserDefined ? 1 : 0));
        parcel.writeTypedList(nestedMsgs);
        parcel.writeStringList(valuesForNative);
    }

    public static final Parcelable.Creator<SMSensorPayloadField> CREATOR = new Parcelable.Creator<SMSensorPayloadField>() {
        @Override
        public SMSensorPayloadField createFromParcel(Parcel parcel) {
            return new SMSensorPayloadField(parcel);
        }

        @Override
        public SMSensorPayloadField[] newArray(int size) {
            return new SMSensorPayloadField[size];
        }
    };

    public SMSensorPayloadField(String inName, SMDataType inSMDataType, SMAccessModifier inAccessModifer,
                                boolean inIsEnum, List<String> inEnumList, boolean inHasDefault,
                                String inDefaultValue, List<String> inValue,
                                boolean inIsUserDefinedMsg, List<SMNestedMsgPayload> inNestedMsgs,
                                List<String> inValuesForNative) {
        this.fieldName = inName;
        this.SMDataType = inSMDataType;
        this.SMAccessModifier = inAccessModifer;
        this.isEnumerated = inIsEnum;
        this.enumValues = inEnumList;
        this.hasDefaultValue = inHasDefault;
        this.defaultValue = inDefaultValue;
        this.value = inValue;
        this.isUserDefined = inIsUserDefinedMsg;
        this.nestedMsgs = inNestedMsgs;
        this.valuesForNative = inValuesForNative;
    }
}
