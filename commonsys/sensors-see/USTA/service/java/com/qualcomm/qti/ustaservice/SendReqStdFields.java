/**
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
package com.qualcomm.qti.ustaservice;

public class SendReqStdFields {
    String batchPeriod;
    String flushPeriod;
    String clientType;
    String wakeupType;
    String flushOnly;
    String maxBatch;
    String isPassive;

    public void setMaxBatch(String maxBatch) {
        this.maxBatch = maxBatch;
    }

    public void setFlushOnly(String flushOnly) {
        this.flushOnly = flushOnly;
    }

    public void setBatchPeriod(String batchPeriod) {
        this.batchPeriod = batchPeriod;
    }

    public void setFlushPeriod(String flushPeriod) {
        this.flushPeriod = flushPeriod;
    }

    public void setClientType(String clientType) {
        this.clientType = clientType;
    }

    public void setWakeupType(String wakeupType) {
        this.wakeupType = wakeupType;
    }
    public String getWakeupType() {
        return this.wakeupType;
    }
    public void setPassive(String isPassive) {
        this.isPassive = isPassive;
    }
}
