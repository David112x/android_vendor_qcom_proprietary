/**
 * Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

public class ReqMsgPayload {
    private String msgName;
    private String msgID;
    private SensorPayloadField[] fields;

    public String getMsgName() {
        return msgName;
    }

    public String getMsgID() {
        return msgID;
    }

    public SensorPayloadField[] getFields() {
        return fields;
    }

    public void setFields(SensorPayloadField[] fields) {
        this.fields = fields;
    }

    public void setMsgID(String msgID) {
        this.msgID = msgID;
    }

    public void setMsgName(String msgName) {
        this.msgName = msgName;
    }

    public int getFieldCount(){
        return fields.length;
    }
    public SensorPayloadField getFieldAt(int pos){
        return fields[pos];
    }
}
