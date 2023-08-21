/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file MetadataJson.java
 */
package com.qualcomm.qti.ssmeditor.ssmeditor;

import android.util.Log;

import com.qualcomm.qti.mmca.vpt.classification.ClassificationBatch;
import com.qualcomm.qti.mmca.vpt.classification.ClassificationResult;
import com.qualcomm.qti.mmca.vpt.classification.Frame;
import com.qualcomm.qti.mmca.vpt.classification.Metadata;

import org.json.*;

public class MetadataJson {

    private static final String TAG = "MetadataJson";

    public static Metadata getMetadataFromJson(String json) {
        Metadata data = new Metadata();

        try {
            // Traverse JSON
            JSONObject dataObj = new JSONObject(json);

            // preamble
            data.setFrameRate(dataObj.getInt(Metadata.JSON_FRAME_RATE));

            JSONArray recordsArr = dataObj.getJSONArray(Metadata.JSON_RECORDS);

            for (int i = 0; i < recordsArr.length(); i++) {
                ClassificationBatch newBatch = new ClassificationBatch();
                JSONObject recordObj = recordsArr.getJSONObject(i);

                JSONArray classResultsArr = recordObj.getJSONArray(
                        ClassificationBatch.JSON_CLASSIFICATION_RESULTS);
                for (int j = 0; j < classResultsArr.length(); j++) {
                    JSONObject classRecord = classResultsArr.getJSONObject(j);
                    String id = classRecord.getString(ClassificationResult.JSON_CLASS_ID);
                    float logit = (float)classRecord.getDouble(ClassificationResult.JSON_LOGIT);
                    newBatch.addClassificationResult(new ClassificationResult(id, logit));
                }

                JSONArray framesArr = recordObj.getJSONArray(ClassificationBatch.JSON_FRAMES);
                for (int j = 0; j < framesArr.length(); j++) {
                    JSONObject frameObj = framesArr.getJSONObject(j);

                    Frame f = new Frame(frameObj.getInt(Frame.JSON_INDEX));
                    f.setDataValid(frameObj.getInt(Frame.JSON_DATA_VALID));
                    f.setVideoTimestamp(frameObj.getLong(Frame.JSON_VIDEO_TIMESTAMP));
                    f.setSystemTimestamp(frameObj.getLong(Frame.JSON_SYSTEM_TIMESTAMP));
                    newBatch.addFrame(f);
                }
                data.addBatch(newBatch);
            }
        } catch (JSONException e) {
            Log.e(TAG, "getMetadataFromJson exception: " + e);
        }
        return data;
    }
}
