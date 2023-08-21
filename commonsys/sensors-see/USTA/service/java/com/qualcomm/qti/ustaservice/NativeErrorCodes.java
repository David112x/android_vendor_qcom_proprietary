/**
 * Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

public enum NativeErrorCodes {
    USTA_MEMORY_ERROR,
    USTA_PROTOFILES_NOT_FOUND,
    USTA_REQUIRED_FIELDS_MISSING,
    USTA_NO_ERROR;

    private int enumValue;

    public int getEnumValue() {
        return enumValue;
    }

    public void setEnumValue(int enumValue) {
        this.enumValue = enumValue;
    }
}
