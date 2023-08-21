/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file Frame.java
 */
package com.qualcomm.qti.mmca.vpt.classification;

import org.json.JSONException;
import org.json.JSONObject;

public class Frame {

    public static String JSON_INDEX             = "index";
    public static String JSON_DATA_VALID        = "data_valid";
    public static String JSON_VIDEO_TIMESTAMP   = "vid_ts";
    public static String JSON_SYSTEM_TIMESTAMP  = "sys_ts";

    private int mIdx;
    private int mDataValid;
    private long mVideoTimestamp;
    private long mSystemTimestamp;

    public Frame(int index) {
        mIdx = index;
    }

    public int getIndex() {
        return mIdx;
    }

    public boolean isValid() {
        return getDataValid() != 0;
    }

    public int getDataValid() {
        return mDataValid;
    }

    public void setDataValid(int newDataValid) {
        mDataValid = newDataValid;
    }

    public long getVideoTimestamp() {
        return mVideoTimestamp;
    }

    public void setVideoTimestamp(long ts) {
        mVideoTimestamp = ts;
    }

    public long getSystemTimestamp() {
        return mSystemTimestamp;
    }

    public void setSystemTimestamp(long ts) {
        mSystemTimestamp = ts;
    }

    public JSONObject asJSONObject() {
        JSONObject j = new JSONObject();
        try {
            j.put(JSON_INDEX, mIdx)
             .put(JSON_DATA_VALID, mDataValid)
             .put(JSON_VIDEO_TIMESTAMP, mVideoTimestamp)
             .put(JSON_SYSTEM_TIMESTAMP, mSystemTimestamp);
        } catch (JSONException e) {
            // what do?
        }
        return j;
    }
}
