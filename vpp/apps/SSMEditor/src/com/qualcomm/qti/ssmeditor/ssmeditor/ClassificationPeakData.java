/* ClassificationPeakData.java
 *
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

package com.qualcomm.qti.ssmeditor.ssmeditor;

public class ClassificationPeakData {

    private int peakLogit;
    private long peakIdx;

    public ClassificationPeakData(int logit, long idx) {
        peakLogit = logit;
        peakIdx = idx;
    }

    public void setPeakLogit(int logit) {
        peakLogit = logit;
    }

    public int getPeakLogit() {
        return peakLogit;
    }

    public void setPeakIdx(long idx) {
        peakIdx = idx;
    }

    public long getPeakIdx() {
        return peakIdx;
    }
}
