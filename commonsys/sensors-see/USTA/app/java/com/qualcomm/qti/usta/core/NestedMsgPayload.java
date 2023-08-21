/*============================================================================
  Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

  @file NestedMsgPayload.java
  @brief
   NestedMsgPayload is mapped to native data structure.
============================================================================*/
package com.qualcomm.qti.usta.core;

import java.util.Arrays;

public class NestedMsgPayload {
  private String nestedMsgName;
  private SensorPayloadField[] fields = {};

  public void setNestedMsgName(String nestedMsgName) {
    this.nestedMsgName = nestedMsgName;
  }
  public void setFields(SensorPayloadField[] fields) {
    this.fields = fields;
  }

  public String getNestedMsgName() {
    return nestedMsgName;
  }

  public SensorPayloadField[] getFields() {
    return fields;
  }

  public int getFieldCount(){
    return fields.length;
  }

  public SensorPayloadField getFiledAt(int pos){
    return fields[pos];
  }

  public void addField(SensorPayloadField payloadField){

    SensorPayloadField currentNewFieldInfo = new SensorPayloadField();

    currentNewFieldInfo.setAccessModifier(payloadField.getAccessModifier());
    currentNewFieldInfo.setDataType(payloadField.getDataType());
    currentNewFieldInfo.setFieldName(payloadField.getFieldName());
    currentNewFieldInfo.setUserDefined(payloadField.isUserDefined());

    currentNewFieldInfo.setEnumerated(payloadField.isEnumerated());
    if(true == payloadField.isEnumerated()) {
      String[] enumValues = {};
      int enumLength = payloadField.getEnumValues().length ;
      enumValues = Arrays.copyOf(enumValues,enumLength);
      for(int enumIndex = 0 ; enumIndex < payloadField.getEnumValues().length ; enumIndex ++){
        enumValues[enumIndex] = payloadField.getEnumValues()[enumIndex];
      }
      currentNewFieldInfo.setEnumValues(enumValues);
    }

    currentNewFieldInfo.setHasDefaultValue(payloadField.isHasDefaultValue());
    if(true == payloadField.isHasDefaultValue()){
      currentNewFieldInfo.setDefaultValue(payloadField.getDefaultValue());
    }

    currentNewFieldInfo.setUserDefined(payloadField.isUserDefined());
    if(true == payloadField.isUserDefined()){
      currentNewFieldInfo.addNestedMsgs();
    }

    final int newLength = this.fields.length + 1;
    this.fields = Arrays.copyOf(this.fields,newLength );
    this.fields[newLength - 1] = currentNewFieldInfo;
  }

}
