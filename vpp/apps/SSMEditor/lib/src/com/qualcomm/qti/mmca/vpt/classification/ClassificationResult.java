/**
 * Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file ClassificationResult.java
 */
package com.qualcomm.qti.mmca.vpt.classification;

import org.json.JSONException;
import org.json.JSONObject;

public class ClassificationResult {
    public static String JSON_CLASS_ID  = "class_id";
    public static String JSON_LOGIT     = "logit";

    private String mClassId;
    private float mLogit;

    public ClassificationResult(String id, float logit) {
        mClassId = id;
        mLogit = logit;
    }

    public String getClassId() {
        return mClassId;
    }

    public float getLogit() {
        return mLogit;
    }

    public JSONObject asJSONObject() {
        JSONObject j = new JSONObject();
        try {
            j.put(JSON_CLASS_ID, mClassId)
             .put(JSON_LOGIT, mLogit);
        } catch (JSONException e) {
            // what do?
        }
        return j;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ClassificationResult) {
            ClassificationResult them = (ClassificationResult)obj;
            return them.mClassId == mClassId && them.mLogit == mLogit;
        }
        return false;
    }
}
