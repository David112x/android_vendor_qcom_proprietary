/**
 * Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

public enum DataType {
    STRING,
    SIGNED_INTEGER32,
    SIGNED_INTEGER64,
    UNSIGNED_INTEGER32,
    UNSIGNED_INTEGER64,
    FLOAT,
    BOOLEAN,
    ENUM,
    USER_DEFINED;

    private int enumValue;

    public int getEnumValue() {
        return enumValue;
    }

    public void setEnumValue(int enumValue) {
        this.enumValue = enumValue;
    }
}
