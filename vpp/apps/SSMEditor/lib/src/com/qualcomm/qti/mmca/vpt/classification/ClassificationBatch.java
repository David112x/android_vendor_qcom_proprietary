/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file ClassificationBatch.java
 */
package com.qualcomm.qti.mmca.vpt.classification;

import java.util.List;
import java.util.Vector;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class ClassificationBatch {

    public static String JSON_FRAMES                    = "frames";
    public static String JSON_CLASSIFICATION_RESULTS    = "classification_results";

    private List<Frame> mFrames = new Vector();
    private List<ClassificationResult> mResults = new Vector();

    public void addFrame(Frame frame) {
        mFrames.add(frame);
    }

    public List<Frame> getFrames() {
        return mFrames;
    }

    public void setFrames(List<Frame> newFrames) {
        mFrames.clear();
        mFrames.addAll(newFrames);
    }

    public long getStartTime() {
        long ts = 0;
        if (!mFrames.isEmpty()) {
            ts = mFrames.get(0).getVideoTimestamp();
        }
        return ts;
    }

    public long getEndTime() {
        long ts = 0;
        if (!mFrames.isEmpty()) {
            ts = mFrames.get(mFrames.size() - 1).getVideoTimestamp();
        }
        return ts;
    }

    public boolean containsAnyValidFrames() {
        if (mFrames.isEmpty()) {
            return false;
        }

        for (Frame f: mFrames) {
            if (f.isValid()) {
                return true;
            }
        }

        return false;
    }

    public void addClassificationResult(ClassificationResult r) {
        mResults.add(r);
    }

    public List<ClassificationResult> getClassificationResults() {
        return mResults;
    }

    public void setClassificationResults(List<ClassificationResult> newResults) {
        mResults.clear();
        mResults.addAll(newResults);
    }

    public JSONObject asJSONObject() {
        JSONObject j = new JSONObject();
        JSONArray jaFrames = new JSONArray();
        JSONArray jaResults = new JSONArray();
        try {
            for (ClassificationResult r : mResults) {
                jaResults.put(r.asJSONObject());
            }
            for (Frame f : mFrames) {
                jaFrames.put(f.asJSONObject());
            }
            j.put(JSON_CLASSIFICATION_RESULTS, jaResults)
             .put(JSON_FRAMES, jaFrames);
        } catch (JSONException e) {
            // what do?
        }
        return j;
    }
}
