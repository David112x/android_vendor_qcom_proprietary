/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file Metadata.java
 */
package com.qualcomm.qti.mmca.vpt.classification;

import android.util.Log;

import java.io.FileWriter;
import java.io.IOException;
import java.util.List;
import java.util.Vector;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class Metadata {

    private final static String TAG = "Metadata";

    public static String JSON_RECORDS       = "records";
    public static String JSON_SESSION_INFO  = "session_info";
    public static String JSON_FRAME_RATE    = "frame_rate";
    public static String JSON_NUM_BATCHES   = "num_batches";

    private static int JSON_INDENTATION_SPACES = 2;

    private List<ClassificationBatch> mBatches = new Vector();
    private SessionInfo mSessionInfo = new SessionInfo();

    private int mFrameRate;

    public void addBatch(ClassificationBatch r) {
        mBatches.add(r);
    }

    public List<ClassificationBatch> getBatches() {
        return mBatches;
    }

    /**
     * Discard invalid batches that come before the first valid batch, keeping
     * only |numToKeepBeforeFirstValid| batches.
     */
    public void pruneLeadingInvalidBatches(int numToKeepBeforeFirstValid) {
        // Linear search should be sufficient here since the use case is HSR
        // and the recording length realistically shouldn't be long enough
        // such that there are enough batches to warrant a more complex search.
        int i;
        for (i = 0; i < mBatches.size(); i++) {
            ClassificationBatch batch = mBatches.get(i);
            if (batch.containsAnyValidFrames()) {
                break;
            }
        }

        int keepIndex = i - numToKeepBeforeFirstValid;
        Log.d(TAG, "numToKeepBeforeFirstValid=" + numToKeepBeforeFirstValid +
                ", i=" + i + ", keepIndex=" + keepIndex);
        if (keepIndex > 0) {
            mBatches = mBatches.subList(keepIndex, mBatches.size());
        }
    }

    public SessionInfo getSessionInfo() {
        return mSessionInfo;
    }

    public void setSessionInfo(SessionInfo si) {
        mSessionInfo = si;
    }

    public int getFrameRate() {
        return mFrameRate;
    }

    public void setFrameRate(int newFrameRate) {
        mFrameRate = newFrameRate;
    }

    public void writeToFileAsJSON(String filepath) throws IOException {
        FileWriter fw = new FileWriter(filepath);
        fw.write(asJSONString());
        fw.close();
    }

    public JSONObject asJSONObject() {
        JSONObject j = new JSONObject();
        JSONArray ja = new JSONArray();
        try {
            for (ClassificationBatch r : mBatches) {
                ja.put(r.asJSONObject());
            }
            j.put(JSON_SESSION_INFO, mSessionInfo.asJSONObject())
             .put(JSON_FRAME_RATE, mFrameRate)
             .put(JSON_NUM_BATCHES, mBatches.size())
             .put(JSON_RECORDS, ja);
        } catch (JSONException e) {
            // what do?
        }
        return j;
    }

    public String asJSONString() {
        try {
            return asJSONObject().toString(JSON_INDENTATION_SPACES);
        } catch (JSONException e) {
            return "";
        }
    }
}
