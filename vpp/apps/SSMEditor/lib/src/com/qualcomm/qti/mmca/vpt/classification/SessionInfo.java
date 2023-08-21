/**
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @file SessionInfo.java
 */
package com.qualcomm.qti.mmca.vpt.classification;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Dictionary;
import java.util.Enumeration;
import java.util.Hashtable;

public class SessionInfo {

    public class SingleSetTimeStat {
        private long mTime = -1;

        public long get() {
            return mTime;
        }

        public void set() {
            set(System.nanoTime());
        }

        public void set(long time) {
            if (mTime == -1) {
                mTime = time;
            }
        }
    }

    public enum Stats {
        StartRepeatingCaptureRequests,
        MediaRecorderStartTime,
        MediaRecorderPauseTime,
        MediaRecorderStopTime,
        VPTFirstClassificationResultTime,
        VPTFirstValidResult,
        VPTBaseTimestamp,

        DEBUG_OPEN_CAMERA_DEVICE_START,
        DEBUG_OPEN_CAMERA_DEVICE_END,
        DEBUG_CREATE_CAMERA_SESSION_START,
        DEBUG_CREATE_CAMERA_SESSION_END,

        DEBUG_CLOSE_CAPTURE_SESSION_START,
        DEBUG_CLOSE_CAPTURE_SESSION_END,
        DEBUG_CLOSE_CAMERA_DEVICE_START,
        DEBUG_CLOSE_CAMERA_DEVICE_END,
        DEBUG_STOP_MEDIA_RECORDER_START,
        DEBUG_STOP_MEDIA_RECORDER_END,
    }

    public static String SESSION_NAME = "SessionName";
    public static String CAPTURE_RATE = "CaptureRate";

    private SingleSetTimeStat[] mEnumeratedStats = new SingleSetTimeStat[Stats.values().length];
    private Dictionary<String, String> mStringStats = new Hashtable<>();

    public SessionInfo() {
        for (int i = 0; i < mEnumeratedStats.length; i++) {
            mEnumeratedStats[i] = new SingleSetTimeStat();
        }
    }

    public void set(String info, String value) {
        mStringStats.put(info, value);
    }

    public void set(Stats stat) {
        mEnumeratedStats[stat.ordinal()].set();
    }

    public void set(Stats stat, long time) {
        mEnumeratedStats[stat.ordinal()].set(time);
    }

    public JSONObject asJSONObject() {
        JSONObject j = new JSONObject();
        try {
            Enumeration<String> keys = mStringStats.keys();
            while (keys.hasMoreElements()) {
                String key = keys.nextElement();
                String value = mStringStats.get(key);
                j.put(key, value);
            }
            for (Stats stat : Stats.values()) {
                j.put(stat.name(), mEnumeratedStats[stat.ordinal()].get());
            }
        } catch (JSONException e) {
            // what do?
        }
        return j;
    }
}
