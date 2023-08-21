/**
 * Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
 
package com.qualcomm.qti.ustaservice;

public enum AccessModifier {
    REQUIRED,
    OPTIONAL,
    REPEATED;

    private int accessModifierValue;


    public int getAccessModifierValue() {
        return accessModifierValue;
    }

    public void setAccessModifierValue(int accessModifierValue) {
        this.accessModifierValue = accessModifierValue;
    }
}
