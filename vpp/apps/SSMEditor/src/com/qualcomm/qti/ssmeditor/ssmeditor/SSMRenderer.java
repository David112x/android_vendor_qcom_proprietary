/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SSMRenderer.java
 */
package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.util.Log;

import com.qualcomm.qti.mmca.ssmeditor.FRCSegment;
import com.qualcomm.qti.ssmeditor.decoder.Renderer;

import java.util.Collections;
import java.util.List;

/**
 * Renderer specific to SSME.
 */
public abstract class SSMRenderer extends Renderer {

    private static final String TAG = "SSMRenderer";
    private static final long SECOND_US = 1000000;
    private static final int TARGET_OUTPUT_FRAME_RATE = 30;

    protected long mTimestamp; // Times in ns

    private List<FRCSegment> mSegmentList;

    public SSMRenderer() {
        mSegmentList = Collections.emptyList();

        mTimestamp = 0;
    }

    public void setSegments(List<FRCSegment> segmentList) {
        if (segmentList == null) {
            mSegmentList = Collections.emptyList();
        } else {
            mSegmentList = segmentList;
        }
    }

    protected long calculateTimestampInterval(long timeUs, int frameRate,
        int baseFrameRate) {

        int factor = getTimescaleFactor(timeUs, frameRate, baseFrameRate);
        long interval = getInterval(frameRate);

        Log.v(TAG, "FACTOR APPLIED = " + factor
                + ", interval = " + interval);

        return factor * interval * 1000;
    }

    private boolean isInSlowZone(long timeUs) {
        for (FRCSegment segment : mSegmentList) {
            if (timeUs >= segment.getStartTime() && timeUs < segment.getEndTime()
                    && !segment.isBypass()) {
                return true;
            }
        }

        return false;
    }

    private int getTimescaleFactor(long timeUs, int frameRate, int baseFrameRate) {
        if (isInSlowZone(timeUs)) {
            return frameRate / TARGET_OUTPUT_FRAME_RATE;
        }

        return frameRate / baseFrameRate;
    }

    private long getInterval(int frameRate) {
        return SECOND_US / frameRate + 1;
    }

}
