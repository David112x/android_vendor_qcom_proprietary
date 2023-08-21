/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Vector;

public class typeCasteUtility {
    private static final String TAG = "USTA_ServiceImpl";
    public static SMNestedMsgPayload NestedMsgPayloadToSMNestedMsgPayload(NestedMsgPayload in) {
        List<SMSensorPayloadField> fields = new ArrayList<SMSensorPayloadField>();
        for(int index = 0 ; index < in.getFieldCount() ; index ++){
            SMSensorPayloadField it = SensorPayloadFieldToSMSensorPayloadField(in.getFiledAt(index));
            fields.add(it);
        }
        SMNestedMsgPayload out = new SMNestedMsgPayload(in.getNestedMsgName(),fields);
        return out;
    }

    public static SMSensorPayloadField SensorPayloadFieldToSMSensorPayloadField(SensorPayloadField in){

        Vector<String> valueVector = in.getValue();
        List<String> valueList = new ArrayList<String>();
        for(int index = 0 ; index < valueVector.size(); index ++) {
            String it = valueVector.elementAt(index);
            valueList.add(it);
        }

        List<SMNestedMsgPayload> nestedMsgs = new ArrayList<SMNestedMsgPayload>();
        for(int index = 0; index < in.getNestedMsgSize(); index++){
            SMNestedMsgPayload it = NestedMsgPayloadToSMNestedMsgPayload(in.getNestedMsgs()[index]);
            nestedMsgs.add(it);
        }

        List<String> enumValueList = new ArrayList<String>();
        if(true == in.isEnumerated()) {
            enumValueList = Arrays.asList(in.getEnumValues());
        }

        List<String> valueForNativeList = new ArrayList<String>();
        if(in.getValuesForNative()  != null )
            if(in.getValuesForNative().length != 0)
                valueForNativeList = Arrays.asList(in.getValuesForNative());

        SMSensorPayloadField out = new SMSensorPayloadField(in.getFieldName(),
                DataTypeToSMDataType(in.getDataType()),
                AccessModifierToSMAccessModifier(in.getAccessModifier()),
                in.isEnumerated(),
                enumValueList,
                in.isHasDefaultValue(),
                in.getDefaultValue(),
                valueList,
                in.isUserDefined(),
                nestedMsgs,
                valueForNativeList
        );
        return out;
    }

    public static SMReqMsgPayload ReqMsgPayloadToSMReqMsgPayload(ReqMsgPayload in) {
        List<SMSensorPayloadField> fieldInfo = new ArrayList<SMSensorPayloadField>();
        for(int fieldIndex = 0 ; fieldIndex < in.getFields().length; fieldIndex++){
            SMSensorPayloadField it = SensorPayloadFieldToSMSensorPayloadField(in.getFields()[fieldIndex]);
            fieldInfo.add(it);
        }
        SMReqMsgPayload out = new SMReqMsgPayload(in.getMsgName(),in.getMsgID(),fieldInfo);
        return out;
    }

    public static SMDataType DataTypeToSMDataType(DataType in) {
        return SMDataType.values()[in.ordinal()];
    }

    public static SMAccessModifier AccessModifierToSMAccessModifier(AccessModifier in) {
        return SMAccessModifier.values()[in.ordinal()];
    }

    public static DataType SMDataTypeToDataType(SMDataType in) {
        return DataType.values()[in.ordinal()];
    }

    public static AccessModifier SMAccessModifierToAccessModifier(SMAccessModifier in) {
        return AccessModifier.values()[in.ordinal()];
    }

    public static ModeType SMModeTypeToModeType(SMModeType in) {
        return ModeType.values()[in.ordinal()];
    }

    public static SMModeType ModeTypeToSMModeType(ModeType in) {
        return SMModeType.values()[in.ordinal()];
    }

    public static SMNativeErrorCodes NativeErrorCodesToSMNativeErrorCodes(NativeErrorCodes in) {
        return SMNativeErrorCodes.values()[in.ordinal()];
    }

    public static NativeErrorCodes SMNativeErrorCodesToNativeErrorCodes(SMNativeErrorCodes in) {
        return NativeErrorCodes.values()[in.ordinal()];
    }


    public static SendReqStdFields SMClientReqMsgFieldsToSendReqStdFields(SMClientReqMsgFields in) {
        SendReqStdFields out = new SendReqStdFields();
        out.batchPeriod = in.batchPeriod;
        out.flushPeriod = in.flushPeriod;
        out.clientType = in.clientType;
        out.wakeupType = in.wakeupType;
        out.flushOnly = in.flushOnly;
        out.maxBatch = in.maxBatch;
        out.isPassive = in.isPassive;
        return out;
    }

    public static SMClientReqMsgFields  SendReqStdFieldsToSMClientReqMsgFields(SendReqStdFields in) {
        SMClientReqMsgFields out = new SMClientReqMsgFields(in.batchPeriod,
                in.flushPeriod,
                in.clientType,
                in.wakeupType,
                in.flushOnly,
                in.maxBatch,
                in.isPassive);
        return out;
    }

    public static NestedMsgPayload SMNestedMsgPayloadToNestedMsgPayload(SMNestedMsgPayload in) {
        NestedMsgPayload out = new NestedMsgPayload();
        out.setNestedMsgName(in.nestedMsgName);

        SensorPayloadField[] fields = new SensorPayloadField[ in.fields.size()];
        for(int fieldIndex = 0 ; fieldIndex < in.fields.size(); fieldIndex++) {
            SensorPayloadField it = SMSensorPayloadFieldToSensorPayloadField(in.fields.get(fieldIndex));
            fields[fieldIndex] = it;
        }
        out.setFields(fields);
        return out;
    }

    public static SensorPayloadField SMSensorPayloadFieldToSensorPayloadField(SMSensorPayloadField in) {

        SensorPayloadField out = new SensorPayloadField();
        out.setFieldName(in.fieldName);
        out.setDataType(SMDataTypeToDataType(in.SMDataType));
        out.setAccessModifier(SMAccessModifierToAccessModifier(in.SMAccessModifier));
        out.setEnumerated(in.isEnumerated);

        /*Convert List to String */
        String[] enumValues = new String[in.enumValues.size()];
        enumValues = in.enumValues.toArray(enumValues);
        out.setEnumValues(enumValues);

        out.setHasDefaultValue(in.hasDefaultValue);
        out.setDefaultValue(in.defaultValue);
        out.setUserDefined(in.isUserDefined);

        String[] valuesForNative = new String[in.valuesForNative.size()];
        valuesForNative = in.valuesForNative.toArray(valuesForNative);
        out.setValuesForNative(valuesForNative);

        Vector<String> value = new Vector<String>();
        for(int index = 0; index < in.value.size(); index ++){
            String it = in.value.get(index);
            value.add(it);
        }
        out.setValue(value);

        NestedMsgPayload[] nestedMsgs = new NestedMsgPayload[in.nestedMsgs.size()];
        for(int index = 0 ; index < in.nestedMsgs.size(); index++) {
            NestedMsgPayload it = SMNestedMsgPayloadToNestedMsgPayload(in.nestedMsgs.get(index));
            nestedMsgs[index] = it;
        }
        out.setNestedMsgs(nestedMsgs);

        return out;
    }

    public static ReqMsgPayload SMReqMsgPayloadToReqMsgPayload(SMReqMsgPayload in) {
        ReqMsgPayload out = new ReqMsgPayload();
        out.setMsgName(in.msgName);
        out.setMsgID(in.msgID);
        SensorPayloadField[] fields = new SensorPayloadField[ in.fieldInfoVar.size()];
        for(int fieldIndex = 0 ; fieldIndex < in.fieldInfoVar.size(); fieldIndex++) {
            SensorPayloadField it = SMSensorPayloadFieldToSensorPayloadField(in.fieldInfoVar.get(fieldIndex));
            fields[fieldIndex] = it;
        }
        out.setFields(fields);
        return out;
    }

}
