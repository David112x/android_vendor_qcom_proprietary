/**
 * Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

public enum ModeType {
    USTA_MODE_TYPE_UI,
    USTA_MODE_TYPE_COMMAND_LINE,
    /*Please add all other modes before this line. USTA_MODE_TYPE_COUNT will be the last line to have count */
    USTA_MODE_TYPE_COUNT;

    private int enumValue;

    public int getEnumValue() {
        return enumValue;
    }

    public void setEnumValue(int enumValue) {
        this.enumValue = enumValue;
    }
}
