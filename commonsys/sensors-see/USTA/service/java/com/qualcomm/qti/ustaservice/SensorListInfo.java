/**
 * Copyright (c) 2017,2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

public class SensorListInfo {

    String dataType;
    String vendor;
    String suidLow;
    String suidHigh;

    public String getDataType() {
        return dataType;
    }

    public String getVendor() {
        return vendor;
    }

    public String getSuidLow() {
        return suidLow;
    }

    public String getSuidHigh() {
        return suidHigh;
    }

    public void setSuidHigh(String suidHigh) {
        this.suidHigh = suidHigh;
    }

    public void setSuidLow(String suidLow) {
        this.suidLow = suidLow;
    }

    public void setVendor(String vendor) {
        this.vendor = vendor;
    }

    public void setDataType(String dataType) {
        this.dataType = dataType;
    }
}
